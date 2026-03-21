#include "DescriptorPool.h"
#include "Logger.h"
#include <unordered_map>
#include <iostream>


namespace caaVk
{
	DescriptorPool::DescriptorPool(VkDevice& device) : device_(device)
	{
	}

	DescriptorPool::~DescriptorPool()
	{
		cleanup();
	}


	/*
	파일을 읽어와서 이전에 할당된 DescriptorSet의 유형과 개수를 기반으로 초기 DescriptorPool을 생성하는 함수
	입니다.
	*/
	void DescriptorPool::createFromScript()
	{
		std::ifstream file(kScriptFilename_);

		if (file.is_open()) {
			printLog("Found DescriptorPoolSize.txt, loading previous statistics...");

			string line;
			vector<VkDescriptorPoolSize> poolSizes;
			uint32_t numSets = 0;

			while (std::getline(file, line)) {
				std::istringstream iss(line);
				string typeStr;
				uint32_t count;

				if (iss >> typeStr >> count) {
					if (typeStr == "NumSets") {
						numSets = count;
					}
					else {
						// Convert string to VkDescriptorType using VulkanTools function
						VkDescriptorType type = stringToDescriptorType(typeStr);
						if (type != VK_DESCRIPTOR_TYPE_MAX_ENUM) {
							VkDescriptorPoolSize poolSize{};
							poolSize.type = type;
							poolSize.descriptorCount = count;
							poolSizes.push_back(poolSize);
						}
					}
				}
			}
			file.close();

			// Create pool with loaded statistics if we have valid data
			if (numSets > 0 && !poolSizes.empty()) {
				createNewPool(poolSizes, numSets);
				printLog("Created initial pool with {} sets and {} descriptor types", numSets,
					poolSizes.size());
			}
		}
		else {
			printLog("DescriptorPoolSize.txt not found, will create pools on-demand");
		}

	}

	void DescriptorPool::createNewPool(const std::vector<VkDescriptorPoolSize>& typeCounts, uint32_t maxSets)
	{
	}

	void DescriptorPool::printAllocatedStatistics() const
	{
	}

	void DescriptorPool::updateRemainingCapacity(const std::vector<VkDescriptorSetLayoutBinding>& bindings, uint32_t numSets)
	{
	}

	auto DescriptorPool::canAllocateFromRemaining(const std::unordered_map<VkDescriptorType, uint32_t>& requiredTypeCounts, uint32_t numRequiredSets) const -> bool
	{
		return false;
	}

	auto DescriptorPool::allocateDescriptorSet(const VkDescriptorSetLayout& descriptorSetLayouts) -> VkDescriptorSet
	{
		return VkDescriptorSet();
	}

	auto DescriptorPool::allocateDescriptorSets(const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts) -> std::vector<VkDescriptorSet>
	{
		return std::vector<VkDescriptorSet>();
	}

	void DescriptorPool::createLayouts(const std::vector<LayoutInfo>& layoutInfos)
	{
	}

	auto DescriptorPool::descriptorSetLayout(const std::vector<VkDescriptorSetLayoutBinding>& bindings) -> const VkDescriptorSetLayout&
	{
		// Search for bindings in layoutsAndInfos_
		for (const auto& [layout, layoutInfo] : layoutsAndInfos_) {
			if (BindingEqual{}(layoutInfo.bindings_,
				bindings)) { // explicitly use operator defined in VulkanTools.h
				return layout;
			}
		}

		exitWithMessage(
			"Failed to find descriptor set layout for the given bindings in layoutsAndInfos_");

		// This should never be reached, but needed for compiler
		static const VkDescriptorSetLayout empty = VK_NULL_HANDLE;
		return empty;
	}

	auto DescriptorPool::layoutToBindings(const VkDescriptorSetLayout& layout) -> const std::vector<VkDescriptorSetLayoutBinding>&
	{
		// Search for layout in layoutAndInfos_
		for (const auto& [storedLayout, layoutInfo] : layoutsAndInfos_) {
			if (storedLayout == layout) {
				return layoutInfo.bindings_;
			}
		}

		exitWithMessage("Failed to find descriptor set layout in layoutAndInfos_");

		static vector<VkDescriptorSetLayoutBinding> empty;
		return empty;
	}

	void DescriptorPool::cleanup()
	{
	}

}
