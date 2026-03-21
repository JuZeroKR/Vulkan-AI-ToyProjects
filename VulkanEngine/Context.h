#pragma once

#include <vulkan/vulkan.h>

#include <vector>
#include <string>
#include <unordered_map>
#include <vector>

#include "VulkanTool.h"
#include "DescriptorPool.h"
#include "Logger.h"
#include "Shader.h"

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
		// VkQueue presentQueue_{VK_NULL_HANDLE}; // 보통 graphicsQueue와 함께 사용
		// 서로 다른 큐 끼리는 병렬로 작업을 수행할 수 있습니다.
		// GPU 성능에 여유가 있다면 최적화에 활용할 수 있습니다.

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

