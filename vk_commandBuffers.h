#pragma once

#include <vector>
#include <vulkan/vulkan.h>

#include "vk_utils.h"

class AVKCommandBuffer {
public:
    /** Constructor */
    AVKCommandBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
    /** Destructor */
    ~AVKCommandBuffer();

    /** Create command pool */
    void createCommandPool();
    /** Create command buffers */
    void createCommandBuffers();
    /** Clear command buffer resources */
    void clear();

    VkCommandPool getCommandPool() const { return commandPool; }
    const std::vector<VkCommandBuffer>& getCommandBuffers() const { return commandBuffers; }

private:
    //Externals
    VkDevice device;
    VkPhysicalDevice physicalDevice;
    VkSurfaceKHR surface;
    //Internals
    VkCommandPool commandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> commandBuffers;//*MML

    /** Find queue families */
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
};
