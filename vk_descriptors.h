#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#include "vk_texture.h"

//Todo -- link GLM and look into utilising the working of these transforms in Auras Structure
//Todo -- MVP needs to be manipulable by the AURA Camera and Transformation interface
/**
 * Make sure the UBO and shader object match so that there is no
 * issue with Shader
 */
struct UniformBufferObjectV {//Todo -- temporary implementation find a way to make this a param
    alignas(16) glm::mat4 mvp;
};

struct UniformBufferObjectF {//Todo -- temporary implementation find a way to make this a param
    float offset;
};

class AVKDescriptors {
public:
    /** Constructor */
    AVKDescriptors(VkDevice device, VkPhysicalDevice physicalDevice);
    /** Destructor */
    ~AVKDescriptors();

    //Getters_Todo --Move to CPP
    VkDescriptorSetLayout getDescriptorSetLayout() const { return descriptorSetLayout; }
    std::vector<VkDescriptorSet> getDescriptorSets() const { return descriptorSets; }
    std::vector<VkDeviceMemory> getUniformBuffersMemoryV() const { return uniformBuffersMemory; } //used to update the UBO data in update...
    std::vector<VkDeviceMemory> getUniformBuffersMemoryF() const { return uniformBuffersMemoryF; } //This is mapped to the fragment shader ...
    std::vector<VkBuffer> getUniformBuffers() const { return uniformBuffers; } //Not used

    /** Create descriptor set layout */
    void createDescriptorSetLayout(); //Before pipeline setup
    /** Create uniform buffers */
    void createUniformBuffers();
    //After model and texture setup
    /** Create descriptor pool */
    void createDescriptorPool(std::vector<VkImage> swapChainImages);
    /** Create descriptor sets */
    void createDescriptorSets(std::vector<AVKTexture*> textures);//*MTEX
    /** Clear descriptor resources */
    void clear();

private:
    VkDevice device;
    VkPhysicalDevice physicalDevice;

    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;//*MML
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;//*MML
    std::vector<VkDescriptorSet> descriptorSets;//*MML

    //UBO for Vertex
    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;

    //UBO for Fragment
    std::vector<VkBuffer> uniformBuffersF;//*UBOF
    std::vector<VkDeviceMemory> uniformBuffersMemoryF;//*UBOF

    /** Create buffer */
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
    /** Find memory type */
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
};
