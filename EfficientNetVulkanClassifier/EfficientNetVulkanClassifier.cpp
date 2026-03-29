
#include <vulkan/vulkan.h>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <cstdio>
#include <filesystem>
#include "VulkanEngine.h"
#include "VulkanEngine/Context.h"
#include "VulkanEngine/Shader.h"
#include <fstream>
#include "Image2D.h"
#include <glm/glm.hpp>
#include <stb_image_write.h>
#include <shellapi.h>

#include "DescriptorSet.h"
#include "ShaderManager.h"
#include "Pipeline.h"
#include "InferenceEngine.h"

using namespace caaVk;
namespace fs = std::filesystem;

int main(int argc, char* argv[])
{
    std::cout << "[EfficientNetClassifier] CLI Mode initialized." << std::endl;

    try {
        Context ctx({}, false);
        std::cout << "[EfficientNetClassifier] Vulkan initialized." << std::endl;
        
        std::string assetsPath = "assets/";
        std::string testImagesPath = fs::exists(assetsPath + "test_images/") ? (assetsPath + "test_images/") : ("../assets/test_images/");
        
        // [0. Initialize ShaderManager and Layouts]
        ShaderManager sm(ctx, assetsPath + "shaders/", { {"restoration", {"test.comp.spv"}} });
        ctx.descriptorPool().createLayouts(sm.layoutInfos());
        VkDescriptorSetLayout descriptorSetLayout = ctx.descriptorPool().layoutsForPipeline("restoration")[0];

        // [1. Prepare AI Engine]
        InferenceEngine aiEngine(ctx);
        if (!aiEngine.loadModel("../AI/efficientnet_b0.param", "../AI/efficientnet_b0.bin")) {
            throw std::runtime_error("Failed to load AI model");
        }
        if (!aiEngine.loadLabels("../AI/imagenet_classes.txt")) {
            std::cout << "[Warning] Failed to load ImageNet labels." << std::endl;
        }

        // [2. Pipeline]
        VkPipelineLayoutCreateInfo pipelineLayoutCI{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
        pipelineLayoutCI.setLayoutCount = 1;
        pipelineLayoutCI.pSetLayouts = &descriptorSetLayout;
        VkPipelineLayout pipelineLayout;
        vkCreatePipelineLayout(ctx.device(), &pipelineLayoutCI, nullptr, &pipelineLayout);

        VkComputePipelineCreateInfo computePipelineCI{ VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
        computePipelineCI.layout = pipelineLayout;
        auto shaderStages = sm.createPipelineShaderStageCIs("restoration");
        computePipelineCI.stage = shaderStages[0];
        VkPipeline pipeline;
        vkCreateComputePipelines(ctx.device(), VK_NULL_HANDLE, 1, &computePipelineCI, nullptr, &pipeline);

        // [3. Resources]
        Image2D inputImage(ctx);
        inputImage.updateUsageFlags(VK_IMAGE_USAGE_STORAGE_BIT);
        
        Image2D outputImage(ctx);
        outputImage.createGeneralStorage(224, 224);

        VkSamplerCreateInfo sInfo{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
        sInfo.magFilter = VK_FILTER_LINEAR;
        sInfo.minFilter = VK_FILTER_LINEAR;
        VkSampler sampler;
        vkCreateSampler(ctx.device(), &sInfo, nullptr, &sampler);

        VkDescriptorSet dSet = ctx.descriptorPool().allocateDescriptorSet(descriptorSetLayout);

        // [4. Batch Process Loop]
        std::cout << "\n>>> Starting BATCH TEST (Target: " << testImagesPath << ")\n" << std::endl;
        
        int count = 0;
        if (!fs::exists(testImagesPath)) {
            std::cout << "[Error] Test images directory not found: " << testImagesPath << std::endl;
        } else {
            for (auto const& entry : fs::directory_iterator(testImagesPath)) {
                if (!entry.is_regular_file()) continue;
                std::string imgPath = entry.path().string();
                std::string fileName = entry.path().filename().string();

                // (A) Load Image
                try {
                    inputImage.createTextureFromImage(imgPath, false, false);
                } catch (...) {
                    std::cout << "[" << fileName << "] FAILED TO LOAD" << std::endl;
                    continue;
                }

                // (B) Update Descriptor Set
                VkDescriptorImageInfo inInfo{ sampler, inputImage.view(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
                VkDescriptorImageInfo outInfo{ VK_NULL_HANDLE, outputImage.view(), VK_IMAGE_LAYOUT_GENERAL };

                VkWriteDescriptorSet writes[2] = {};
                writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                writes[0].dstSet = dSet; writes[0].dstBinding = 0;
                writes[0].descriptorCount = 1; writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                writes[0].pImageInfo = &inInfo;

                writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                writes[1].dstSet = dSet; writes[1].dstBinding = 1;
                writes[1].descriptorCount = 1; writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                writes[1].pImageInfo = &outInfo;

                vkUpdateDescriptorSets(ctx.device(), 2, writes, 0, nullptr);

                // (C) Dispatch
                auto cmd = ctx.createComputeCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
                inputImage.transitionTo(cmd.handle(), VK_ACCESS_2_SHADER_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
                outputImage.transitionTo(cmd.handle(), VK_ACCESS_2_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);

                vkCmdBindPipeline(cmd.handle(), VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
                vkCmdBindDescriptorSets(cmd.handle(), VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 1, &dSet, 0, nullptr);
                vkCmdDispatch(cmd.handle(), 14, 14, 1);

                outputImage.transitionTo(cmd.handle(), VK_ACCESS_2_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_PIPELINE_STAGE_2_TRANSFER_BIT);
                cmd.submitAndWait();

                // (D) AI Inference
                std::vector<unsigned char> pData(224 * 224 * 4);
                outputImage.downloadToPixels(pData.data());
                ncnn::Mat res = aiEngine.run(224, 224, pData.data());

                if (!res.empty()) {
                    float max_v = -100.0f; int max_i = -1;
                    for (int i = 0; i < res.w; i++) {
                        if (res[i] > max_v) { max_v = res[i]; max_i = i; }
                    }
                    std::cout << "[" << std::setw(3) << ++count << "] " << std::setw(25) << fileName 
                              << " => Result: " << aiEngine.getLabel(max_i) 
                              << " (" << max_v << ")" << std::endl;
                }
            }
        }

        std::cout << "\n>>> Batch Test Complete! Processed " << count << " images." << std::endl;
        system("pause");

        // [Cleanup]
        vkDestroySampler(ctx.device(), sampler, nullptr);
        vkDestroyPipeline(ctx.device(), pipeline, nullptr);
        vkDestroyPipelineLayout(ctx.device(), pipelineLayout, nullptr);

    } catch (const std::exception& e) {
        std::cerr << "[Error] " << e.what() << std::endl;
        system("pause");
        return -1;
    }

    return 0;
}
