#pragma once
//----------------------------------------------------------
#include <cstdint>
#include <vector>
#include <vulkan/vulkan.h>

#include "vk_utils.h"
//----------------------------------------------------------

class AVKTexture {
public:
    /** Constructor */
    AVKTexture(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue graphicsQueue, AssetManager* assetManager);
    /** Destructor */
    ~AVKTexture();

    /** Load texture from file */
    void loadTexture(const char* filename);
    /** Load texture from image data */
    void loadTexture(std::vector<uint8_t> imageData);
    /** Clear texture resources */
    void clear();

    //This is required in descriptor set definition which will get used in the command buffer recordings.
    /** Get Vulkan sampler */
    VkSampler getVKSampler() const { return textureSampler; }
    /** Get image view */
    VkImageView getImageView() const { return textureImageView; }

    /** Set resolution mapping factor */
    void setResMappingFactor(float value);

private:
    //Internals
    VkSampler textureSampler = VK_NULL_HANDLE; //used in createDescriptorSets()
    VkImageView textureImageView = VK_NULL_HANDLE; //used in createDescriptorSets()
    int textureWidth = 0;
    int textureHeight = 0;
    int textureChannels = 0;
    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingMemory = VK_NULL_HANDLE;
    VkImage textureImage = VK_NULL_HANDLE;
    VkDeviceMemory textureImageMemory = VK_NULL_HANDLE;

    float resMappingFactor = 1.0f; // this value manipulates the texture image resolution before submission to descriptor.

    //Externals
    VkDevice device;
    VkPhysicalDevice physicalDevice;
    VkCommandPool commandPool;
    VkQueue graphicsQueue;
    AssetManager* assetManager;

    /** Find memory type */
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    /** Load binary file to vector */
    std::vector<uint8_t> LoadBinaryFileToVector(const char* filePath, AssetManager* assetManager);

    /** Decode image */
    void decodeImage(std::vector<uint8_t> texData);
    /** Create texture image */
    void createTextureImage();
    /** Copy buffer to image */
    void copyBufferToImage();
    /** Create texture image views */
    void createTextureImageViews();
    /** Create texture sampler */
    void createTextureSampler();
};
