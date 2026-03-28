#include "../include/VulkanEngine.h"
#include <iostream>
#include <vector>
#include <stdexcept>

namespace caaVk {
    VulkanEngine::VulkanEngine() : instance(VK_NULL_HANDLE) {
    }

    VulkanEngine::~VulkanEngine() {
        cleanup();
    }

    void VulkanEngine::init() {
        createInstance();
    }

    void VulkanEngine::cleanup() {
        if (instance != VK_NULL_HANDLE) {
            vkDestroyInstance(instance, nullptr);
            instance = VK_NULL_HANDLE;
        }
    }

    void VulkanEngine::createInstance() {
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Vulkan Engine App";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "VulkanEngine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        // Required extensions for Windows surface creation
        std::vector<const char*> extensions = { "VK_KHR_surface", "VK_KHR_win32_surface" };
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();
        createInfo.enabledLayerCount = 0;

        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create Vulkan instance!");
        }
    }
}
