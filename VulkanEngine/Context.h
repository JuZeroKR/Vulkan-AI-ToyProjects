#pragma once

#include <vulkan/vulkan.h>

#include <vector>
#include <string>
#include <unordered_map>
#include <vector>

#include "VulkanTool.h"
#include "DescriptorPool.h"
#include "CommandBuffer.h"
#include "Logger.h"

namespace caaVk {
	using namespace std;

	// QueueFamilyIndices
	struct QueueFamilyIndices
	{
		uint32_t graphics = uint32_t(-1);
		uint32_t compute = uint32_t(-1);
		uint32_t transfer = uint32_t(-1);
	};

	class Context
	{
	public:
		Context(const vector<const char*>& requiredInstanceExtensions, bool useSwapchain);
		~Context();

		void cleanup();
		void createQueues();
		void createInstance(vector<const char*> instanceExtensions);
		void createLogicalDevice(bool useSwapChain);
		void createPipelineCache();
		void determineDepthStencilFormat();
		void selectPhysicalDevice();

		auto device() -> VkDevice;
		auto instance() -> VkInstance;
		auto physicalDevice() -> VkPhysicalDevice;

		auto graphicsCommandPool() const -> VkCommandPool;
		auto computeCommandPool() const -> VkCommandPool;
		auto transferCommandPool() const -> VkCommandPool;

		auto descriptorPool() -> DescriptorPool&
		{
			return descriptorPool_;
		}

		auto getMemoryTypeIndex(uint32_t typeFilter, VkMemoryPropertyFlags properties) -> uint32_t;

		auto createTransferCommandBuffer(VkCommandBufferLevel level, bool begin = false) -> CommandBuffer;
		auto createComputeCommandBuffer(VkCommandBufferLevel level, bool begin = false) -> CommandBuffer;

		auto pipelineCache() -> VkPipelineCache { return pipelineCache_; }


	private:
		VkDevice _device;
		VkInstance _vkInstance;
		VkPhysicalDevice _physicalDevice;

		VkCommandPool graphicsCommandPool_{ VK_NULL_HANDLE };
		VkCommandPool computeCommandPool_{ VK_NULL_HANDLE };
		VkCommandPool transferCommandPool_{ VK_NULL_HANDLE };

		// Queue handles
		VkQueue graphicsQueue_{ VK_NULL_HANDLE };
		VkQueue computeQueue_{ VK_NULL_HANDLE };
		VkQueue transferQueue_{ VK_NULL_HANDLE };
		// VkQueue presentQueue_{VK_NULL_HANDLE}; // 蹂댄넻 graphicsQueue? ?④퍡 ?ъ슜
		// ?쒕줈 ?ㅻⅨ ???쇰━??蹂묐젹濡??묒뾽???섑뻾?????덉뒿?덈떎.
		// GPU ?깅뒫???ъ쑀媛 ?덈떎硫?理쒖쟻?붿뿉 ?쒖슜?????덉뒿?덈떎.

		VkPipelineCache pipelineCache_{ VK_NULL_HANDLE };

		QueueFamilyIndices queueFamilyIndices_{};
		VkPhysicalDeviceFeatures enabledFeatures_{};

		vector<VkQueueFamilyProperties> queueFamilyProperties_{};
		vector<string> supportedExtensions_{};
		vector<const char*> enabledDeviceExtensions_{};

		VkPhysicalDeviceProperties deviceProperties_{};
		VkPhysicalDeviceFeatures deviceFeatures_{};
		VkPhysicalDeviceMemoryProperties deviceMemoryProperties_{};

		VkFormat depthFormat_{ VK_FORMAT_UNDEFINED };

		DescriptorPool descriptorPool_;

		bool extensionSupported(string extension);

		auto getQueueFamilyIndex(VkQueueFlags queueFlags) const -> uint32_t;
		auto createCommandPool(uint32_t queueFamilyIndex,
			VkCommandPoolCreateFlags createFlags =
			VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT) -> VkCommandPool;

	};

}

