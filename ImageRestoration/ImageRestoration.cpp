#include "framework.h"
#include "ImageRestoration.h"

#include "spirv_reflect.h"

#include <vulkan/vulkan.h>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <cstdio>
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

using namespace caaVk;


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    // 0. Open debug console window
    AllocConsole();
    FILE* fp = nullptr;
    freopen_s(&fp, "CONOUT$", "w", stdout);
    freopen_s(&fp, "CONOUT$", "w", stderr);
    std::cout << "[ImageRestoration] Console initialized." << std::endl;

    // 1. Initialize Windows Window
    const wchar_t CLASS_NAME[] = L"ImageRestorationWindowClass";

    WNDCLASSW wc = { };
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClassW(&wc);

    HWND hWnd = CreateWindowExW(
        0,
        CLASS_NAME,
        L"ImageRestoration Vulkan Window",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        nullptr, nullptr, hInstance, nullptr
    );

    if (!hWnd)
    {
        return 0;
    }

    ShowWindow(hWnd, nCmdShow);

    // 2. Create Vulkan Context and call createInstance
    std::vector<const char*> instanceExtensions = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        "VK_KHR_win32_surface"
    };

    std::unique_ptr<caaVk::Context> context;
	std::unique_ptr<caaVk::Shader> computeShader;
    try {
        //context = std::make_unique<caaVk::Context>(instanceExtensions, /*useSwapchain=*/true);
        Context ctx({}, false);
        auto device = ctx.device();

        std::cout << "[ImageRestoration] Vulkan instance created successfully." << std::endl;
        std::string assetsPath = "assets/";
        std::string computeShaderFilename = assetsPath + "shaders/preprocess.comp.spv";
        string inputImageFilename = assetsPath + "image.jpg";
        string outputImageFilename = assetsPath + "output.jpg";
        // Image를 GPU까지 올리는데 성공!
		Image2D inputImage(ctx);
        inputImage.updateUsageFlags(VK_IMAGE_USAGE_STORAGE_BIT); // Enable storage image access
        inputImage.createTextureFromImage(inputImageFilename, false, false);
        
        // 출력 이미지 준비 및 쉐이더 로드
		Image2D outputImage(ctx);
        // Image 사이즈 224 x 224로 고정
        outputImage.createGeneralStorage(224, 224);

        // [1. 디스크립터 레이아웃 설정]

		// Descriptor Set Layout 준비 (GPU에 리소스 사용하려면 필수! Where to find resources)
        // 1 Descriptor -> 1 Resource
		// Command Buffer에서 Descriptor Set을 바인딩할 때, 각 Descriptor가 어떤 종류의 리소스를 참조하는지 알려주는 역할
        // 셰이더와 외부 데이터(GPU 메모리)를 연결하는 통로의 설계도
        VkDescriptorSetLayoutBinding bindings[2] = {};
        // 이미지를 읽기 전용으로 사용하며, 필터링(Linear, Nearest 등)이 포함된 샘플러와 이미지가 결합된 형태입니다.
        bindings[0] = { 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr };
        // 쉐이더에서 이미지에 직접 값을 쓰거나(Write), 임의의 위치를 읽을 때 사용합니다. 주로 계산 결과를 저장할 때 씁니다
        bindings[1] = { 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr };

        VkDescriptorSetLayoutCreateInfo layoutCI{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
        layoutCI.bindingCount = 2;
        layoutCI.pBindings = bindings;
        VkDescriptorSetLayout descriptorSetLayout;
        vkCreateDescriptorSetLayout(ctx.device(), &layoutCI, nullptr, &descriptorSetLayout);


        // [2. 파이프라인 레이아웃 & 파이프라인 생성]
        VkPipelineLayoutCreateInfo pipelineLayoutCI{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
        pipelineLayoutCI.setLayoutCount = 1;
        pipelineLayoutCI.pSetLayouts = &descriptorSetLayout;
        VkPipelineLayout pipelineLayout;
        vkCreatePipelineLayout(ctx.device(), &pipelineLayoutCI, nullptr, &pipelineLayout);

        VkComputePipelineCreateInfo computePipelineCI{ VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
        computePipelineCI.layout = pipelineLayout;

        VkPipelineShaderStageCreateInfo shaderStageCI{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
        shaderStageCI.stage = VK_SHADER_STAGE_COMPUTE_BIT;      // 컴퓨트 셰이더임을 명시
        shaderStageCI.module = computeShader->shaderModule();   // 읽어온 모듈 핸들
        shaderStageCI.pName = "main";                           // 시작 함수 이름 (대부분 "main")
        computePipelineCI.stage = shaderStageCI;

        VkPipeline pipeline;
        vkCreateComputePipelines(ctx.device(), VK_NULL_HANDLE, 1, &computePipelineCI, nullptr, &pipeline);

        // 이때 Layout을 넣는구나!
        // [3. 디스크립터 세트 준비 - 수동 버전]
        // ---------------------------------------------------------
        // (1) 빈 디스크립터 세트를 할당 받습니다 (공장에서 빈 상자 하나 가져오기)

        VkDescriptorSet descriptorSet = ctx.descriptorPool().allocateDescriptorSet(descriptorSetLayout);
        // (2) 입력 이미지(0번)를 위한 정보 준비

        // [1. 수동으로 샘플러 생성]
        VkSamplerCreateInfo samplerInfo{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
        samplerInfo.magFilter = VK_FILTER_LINEAR;             // 확대 시 선형 필터링
        samplerInfo.minFilter = VK_FILTER_LINEAR;             // 축소 시 선형 필터링
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE; // 이미지 경계 처리
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        VkSampler defaultSampler;
        vkCreateSampler(ctx.device(), &samplerInfo, nullptr, &defaultSampler);

		// DescriptorImageInfo는 디스크립터 세트에 이미지를 연결할 때 필요한 정보를 담는 구조체입니다.
        VkDescriptorImageInfo inputImageInfo{};
        inputImageInfo.imageView = inputImage.view();   // 이미지 뷰
        inputImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; // 읽기 전용 레이아웃
        inputImageInfo.sampler = defaultSampler;         // (가정: Context에 샘플러가 있다고 할 때)
        // ※ 만약 ctx.sampler()가 없다면, 직접 만든 VkSampler 핸들을 넣어줘야 합니다.
        // (3) 출력 이미지(1번)를 위한 정보 준비
        VkDescriptorImageInfo outputImageInfo{};
        outputImageInfo.imageView = outputImage.view(); // 결과 이미지 뷰
        outputImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL; // 쓰기 가능(Storage) 레이아웃
        outputImageInfo.sampler = VK_NULL_HANDLE;        // 스토리지 이미지는 샘플러가 필요 없습니다.

        // (4) 상자에 어떤 내용물을 채울지 기술하는 '주문서(Write)'를 작성합니다.
        VkWriteDescriptorSet descriptorWrites[2] = {};
        // [입력 이미지 연결]
        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = descriptorSet;
        descriptorWrites[0].dstBinding = 0;              // 셰이더의 binding = 0
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[0].pImageInfo = &inputImageInfo;
        // [출력 이미지 연결]
        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = descriptorSet;
        descriptorWrites[1].dstBinding = 1;              // 셰이더의 binding = 1
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        descriptorWrites[1].pImageInfo = &outputImageInfo;
        // (5) GPU에게 "이 세트(상자)에 이 내용물들을 채워넣어라!"라고 업데이트를 명령합니다.
        vkUpdateDescriptorSets(ctx.device(), 2, descriptorWrites, 0, nullptr);


        // ---------------------------------------------------------
        // [4. 실행(Dispatch) 명령 기록]
        // ---------------------------------------------------------
        auto cmd = ctx.createComputeCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
        // 파이프라인(요리법) 바인딩
        vkCmdBindPipeline(cmd.handle(), VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
        // 방금 채운 디스크립터 세트(재료 상자) 바인딩
        vkCmdBindDescriptorSets(cmd.handle(), VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout,
            0, 1, &descriptorSet, 0, nullptr);
        // 연산 시작 (224x224 이미지를 16x16 워크그룹으로 분할)
        vkCmdDispatch(cmd.handle(), 14, 14, 1);
        // 명령 제출 및 완료 대기
        cmd.submitAndWait();
        std::cout << "[ImageRestoration] Preprocessing successfully executed!" << std::endl;

    }
    catch (const std::exception& e) {
        std::cerr << "[ImageRestoration] Error: " << e.what() << std::endl;
        OutputDebugStringA(e.what());
        return -1;
    }

    // 3. Main Message Loop
    MSG msg = { };
    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            // TODO: Vulkan Render Code Here
        }
    }

    // 4. Cleanup
    context.reset();
    std::cout << "[ImageRestoration] Shutdown complete." << std::endl;

    return (int) msg.wParam;
}
