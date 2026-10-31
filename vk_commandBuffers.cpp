#include "vk_commandBuffers.h"

#include "vk_debughandler.h"

/** Constructor */
AVKCommandBuffer::AVKCommandBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
    : device(device), physicalDevice(physicalDevice), surface(surface) {
}

/** Destructor */
AVKCommandBuffer::~AVKCommandBuffer() {
    clear();
}

/*---------------------------------------------------------------------------------------------
██████╗ ██████╗ ███╗   ███╗███╗   ███╗ █████╗ ███╗   ██╗██████╗
██╔════╝██╔═══██╗████╗ ████║████╗ ████║██╔══██╗████╗  ██║██╔══██╗
██║     ██║   ██║██╔████╔██║██╔████╔██║███████║██╔██╗ ██║██║  ██║
██║     ██║   ██║██║╚██╔╝██║██║╚██╔╝██║██╔══██║██║╚██╗██║██║  ██║
╚██████╗╚██████╔╝██║ ╚═╝ ██║██║ ╚═╝ ██║██║  ██║██║ ╚████║██████╔╝
 ╚═════╝ ╚═════╝ ╚═╝     ╚═╝╚═╝     ╚═╝╚═╝  ╚═╝╚═╝  ╚═══╝╚═════╝

██████╗ ██╗   ██╗███████╗███████╗███████╗██████╗
██╔══██╗██║   ██║██╔════╝██╔════╝██╔════╝██╔══██╗
██████╔╝██║   ██║█████╗  █████╗  █████╗  ██████╔╝
██╔══██╗██║   ██║██╔══╝  ██╔══╝  ██╔══╝  ██╔══██╗
██████╔╝╚██████╔╝██║     ██║     ███████╗██║  ██║
╚═════╝  ╚═════╝ ╚═╝     ╚═╝     ╚══════╝╚═╝  ╚═╝
---------------------------------------------------------------------------------------------*/
/**
 * Command Buffer - create, record and Draw
 * VkCommandPool is a simple object that is used to allocate CommandBuffers.
 * It is connected to a specific Queue Family.
 *
 * A VkCommandBuffer is a Vulkan object that represents a list of commands that the GPU will execute.
 * It is a low-level object that provides fine-grained control over the GPU.
 *
 * We have to create a command pool before we can create command buffers.
 * Command pools manage the memory that is used to store the buffers and command buffers are allocated from them.
 */
void AVKCommandBuffer::createCommandPool() {//After pipeline , before texture
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = static_cast<uint32_t>(indices.graphicsFamily);
    AVKDebug::assertVKSuccess(vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool), "create command pool");
}

/**
 *
 */
void AVKCommandBuffer::createCommandBuffers() { //Last step Before Record and Draw
    //create command buffers
    commandBuffers.resize(MAX_FRAMES_IN_FLIGHT); //Todo -- Every frame requires separate command buffer configuration , could be same or different

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());
    AVKDebug::assertVKSuccess(vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()), "allocate command buffers");
}

/** Clear command buffer resources */
void AVKCommandBuffer::clear() {
    if (commandPool != VK_NULL_HANDLE) {
        vkDestroyCommandPool(device, commandPool, nullptr);
        commandPool = VK_NULL_HANDLE;
    }
    commandBuffers.clear();
}

/** Get queue family */
QueueFamilyIndices AVKCommandBuffer::findQueueFamilies(VkPhysicalDevice device) {
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    for (uint32_t i = 0; i < queueFamilyCount; i++) {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = static_cast<int>(i);
        }
        VkBool32 presentSupport = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
        if (presentSupport) {
            indices.presentFamily = static_cast<int>(i);
        }
        if (indices.isComplete()) {
            break;
        }
    }
    return indices;
}
