#pragma once


#include "VulkanTool.h"
#include "Logger.h"
#include <vector>
#include <unordered_map>
#include <map>


struct LayoutInfo
{
    std::vector<VkDescriptorSetLayoutBinding> bindings_{};
    std::vector<std::tuple<std::string, uint32_t>> pipelineNamesAndSetNumbers_;
};
namespace caaVk {

    class DescriptorPool
    {
        friend class Context;

    public:
        DescriptorPool(VkDevice& device);
        ~DescriptorPool();

        void createFromScript();
        void createNewPool(const std::vector<VkDescriptorPoolSize>& typeCounts, uint32_t maxSets);
        void printAllocatedStatistics() const;
        void updateRemainingCapacity(const std::vector<VkDescriptorSetLayoutBinding>& bindings,
            uint32_t numSets);
        auto
            canAllocateFromRemaining(const std::unordered_map<VkDescriptorType, uint32_t>& requiredTypeCounts,
                uint32_t numRequiredSets) const -> bool;
        auto allocateDescriptorSet(const VkDescriptorSetLayout& descriptorSetLayouts)
            -> VkDescriptorSet;
        auto allocateDescriptorSets(const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts)
            -> std::vector<VkDescriptorSet>;
        void createLayouts(const std::vector<LayoutInfo>& layoutInfos);

        std::vector<VkDescriptorSetLayout> layoutsForPipeline(std::string pipelineName)
        {
            std::vector<VkDescriptorSetLayout> layouts;

            for (const auto& [layout, layoutInfo] : layoutsAndInfos_) {
                // Check if this layout is used by the specified pipeline
                for (const auto& [storedPipelineName, setNumber] :
                    layoutInfo.pipelineNamesAndSetNumbers_) {
                    if (storedPipelineName == pipelineName) {
                        // std::cout << pipelineName << " " << setNumber << std::endl;
                        if (layouts.size() < setNumber + 1) {
                            layouts.resize(setNumber + 1);
                        }
                        layouts[setNumber] = layout;
                    }
                }
            }

            return layouts;
        }

        auto descriptorSetLayout(const std::vector<VkDescriptorSetLayoutBinding>& bindings)
            -> const VkDescriptorSetLayout&;
        auto layoutToBindings(const VkDescriptorSetLayout& layout)
            -> const std::vector<VkDescriptorSetLayoutBinding>&;

    private:
        const std::string kScriptFilename_ = "DescriptorPoolSize.txt";

        VkDevice& device_;
        std::vector<VkDescriptorPool> descriptorPools_{};

        // Usage tracking
        std::unordered_map<VkDescriptorType, uint32_t> allocatedTypeCounts_{};
        std::unordered_map<VkDescriptorType, uint32_t> remainingTypeCounts_{};
        uint32_t allocatedSets_ = 0;
        uint32_t remainingSets_ = 0;

        void cleanup();

        std::vector<std::tuple<VkDescriptorSetLayout, LayoutInfo>> layoutsAndInfos_{};
    };
}
