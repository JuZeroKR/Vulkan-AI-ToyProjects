#pragma once

#include <vulkan/vulkan.h>

namespace caaVk {
    class VulkanEngine {
    public:
        VulkanEngine();
        ~VulkanEngine();

        void init();
        void cleanup();

        VkInstance getInstance() const { return instance; }

    private:
        void createInstance();

        VkInstance instance;
    };
}
