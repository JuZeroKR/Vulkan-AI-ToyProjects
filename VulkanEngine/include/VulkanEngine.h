#pragma once

#include <vulkan/vulkan.h>

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
