#include "vk_texture.h"

#include <cstring>
#include <stdexcept>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "vk_debughandler.h"

namespace {

/** Begin single time commands */
VkCommandBuffer beginSingleTimeCommands(VkDevice device, VkCommandPool commandPool) {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    AVKDebug::assertVKSuccess(vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer), "allocate one-time command buffer");

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    AVKDebug::assertVKSuccess(vkBeginCommandBuffer(commandBuffer, &beginInfo), "begin one-time command buffer");
    return commandBuffer;
}

/** End single time commands */
void endSingleTimeCommands(VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue, VkCommandBuffer commandBuffer) {
    AVKDebug::assertVKSuccess(vkEndCommandBuffer(commandBuffer), "end one-time command buffer");

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    AVKDebug::assertVKSuccess(vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE), "submit one-time command buffer");
    vkQueueWaitIdle(graphicsQueue);
    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

/** Transition image layout */
void transitionImageLayout(VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(device, commandPool);

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage = 0;
    VkPipelineStageFlags destinationStage = 0;
    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }

    vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    endSingleTimeCommands(device, commandPool, graphicsQueue, commandBuffer);
}

}

/** Constructor */
AVKTexture::AVKTexture(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue graphicsQueue, AssetManager* assetManager)
    : device(device), physicalDevice(physicalDevice), commandPool(commandPool), graphicsQueue(graphicsQueue), assetManager(assetManager) {
}

/** Destructor */
AVKTexture::~AVKTexture() {
    clear();
}

/**
 * Apply Texture
 */
void AVKTexture::loadTexture(const char* filename) {
    decodeImage(LoadBinaryFileToVector(filename, assetManager));
}

void AVKTexture::loadTexture(std::vector<uint8_t> imageData) {
    decodeImage(std::move(imageData));
}

/** Clear texture resources */
void AVKTexture::clear() {
    if (textureSampler != VK_NULL_HANDLE) {
        vkDestroySampler(device, textureSampler, nullptr);
        textureSampler = VK_NULL_HANDLE;
    }
    if (textureImageView != VK_NULL_HANDLE) {
        vkDestroyImageView(device, textureImageView, nullptr);
        textureImageView = VK_NULL_HANDLE;
    }
    if (textureImage != VK_NULL_HANDLE) {
        vkDestroyImage(device, textureImage, nullptr);
        textureImage = VK_NULL_HANDLE;
    }
    if (textureImageMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, textureImageMemory, nullptr);
        textureImageMemory = VK_NULL_HANDLE;
    }
    if (stagingBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        stagingBuffer = VK_NULL_HANDLE;
    }
    if (stagingMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, stagingMemory, nullptr);
        stagingMemory = VK_NULL_HANDLE;
    }
}

/** Set resolution mapping factor */
void AVKTexture::setResMappingFactor(float value) {
    resMappingFactor = value;
}

/** Setup for base64 IO conversion and consumption. */
void AVKTexture::decodeImage(std::vector<uint8_t> texData) { //*B64
    int width = 0;
    int height = 0;
    int channels = 0;
    stbi_uc* pixels = stbi_load_from_memory(texData.data(), static_cast<int>(texData.size()), &width, &height, &channels, STBI_rgb_alpha);
    if (pixels == nullptr) {
        throw std::runtime_error("failed to decode texture image");
    }
    textureWidth = width;
    textureHeight = height;
    textureChannels = channels;

    VkDeviceSize imageSize = static_cast<VkDeviceSize>(width) * static_cast<VkDeviceSize>(height) * 4;

    /*
     * Create a staging buffer to hold the pixel data and transfer it to the device local image.
     * The staging buffer is a host-visible buffer that can be mapped and written to by the CPU.
     */
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = imageSize;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    AVKDebug::assertVKSuccess(vkCreateBuffer(device, &bufferInfo, nullptr, &stagingBuffer), "create texture staging buffer");

    VkMemoryRequirements memRequirements{};
    vkGetBufferMemoryRequirements(device, stagingBuffer, &memRequirements);
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits,
                                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    AVKDebug::assertVKSuccess(vkAllocateMemory(device, &allocInfo, nullptr, &stagingMemory), "allocate texture staging memory");
    vkBindBufferMemory(device, stagingBuffer, stagingMemory, 0);

    void* data = nullptr;
    vkMapMemory(device, stagingMemory, 0, imageSize, 0, &data);
    /* We can then directly copy the pixel values into the buffer */
    std::memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(device, stagingMemory);
    stbi_image_free(pixels);//Don't forget to clean up the original pixel array now:

    createTextureImage();
    createTextureImageViews();
    createTextureSampler();

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingMemory, nullptr);
    stagingBuffer = VK_NULL_HANDLE;
    stagingMemory = VK_NULL_HANDLE;
}

/** Create texture image */
void AVKTexture::createTextureImage() { //*TEX
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = static_cast<uint32_t>(textureWidth);
    imageInfo.extent.height = static_cast<uint32_t>(textureHeight);
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    AVKDebug::assertVKSuccess(vkCreateImage(device, &imageInfo, nullptr, &textureImage), "create texture image");

    VkMemoryRequirements memRequirements{};
    vkGetImageMemoryRequirements(device, textureImage, &memRequirements);
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    AVKDebug::assertVKSuccess(vkAllocateMemory(device, &allocInfo, nullptr, &textureImageMemory), "allocate texture image memory");
    vkBindImageMemory(device, textureImage, textureImageMemory, 0);

    transitionImageLayout(device, commandPool, graphicsQueue, textureImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    copyBufferToImage();
    transitionImageLayout(device, commandPool, graphicsQueue, textureImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

/** Copy buffer to image */
void AVKTexture::copyBufferToImage() { //*TEX
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(device, commandPool);

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {static_cast<uint32_t>(textureWidth), static_cast<uint32_t>(textureHeight), 1};
    vkCmdCopyBufferToImage(commandBuffer, stagingBuffer, textureImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    endSingleTimeCommands(device, commandPool, graphicsQueue, commandBuffer); //---End Command Buffer
}

/*
 * Next, create VkImageView and VkSampler which can be used by the fragment shader to sample the color for each of the rendered pixels.
 * VkImageView is a wrapper on top of VkImage. It holds the information about how to interpret the texture. It holds information about how to access the texture. For example, or how to tile da...
 */
/** Create texture image views */
void AVKTexture::createTextureImageViews() { //*TEX
    //Add a copy/memory buffer to hold a VkImageView for the texture and create a new imageview when updating
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = textureImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    AVKDebug::assertVKSuccess(vkCreateImageView(device, &viewInfo, nullptr, &textureImageView), "create texture image view");
}

//We will also need to create a sampler in this to our shader.
/** Create texture sampler */
void AVKTexture::createTextureSampler() { //*TEX
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_FALSE; //Todo -- turn on anisotropic filtering
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;
    AVKDebug::assertVKSuccess(vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler), "create texture sampler");
}

/** Load binary file to vector */
std::vector<uint8_t> AVKTexture::LoadBinaryFileToVector(const char* filePath, AssetManager* assetManager) {
    return ::LoadBinaryFileToVector(filePath, assetManager);
}

/** Find memory type */
uint32_t AVKTexture::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties{};
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    throw std::runtime_error("no suitable memory type for texture");
}
