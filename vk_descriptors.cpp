#include "vk_descriptors.h"

#include <algorithm>
#include <array>

#include "vk_debughandler.h"

/** Constructor */
AVKDescriptors::AVKDescriptors(VkDevice device, VkPhysicalDevice physicalDevice)
    : device(device), physicalDevice(physicalDevice) {
}

/** Destructor */
AVKDescriptors::~AVKDescriptors() {
    clear();
}

void AVKDescriptors::createDescriptorSetLayout() {//*SL//*MML
    VkDescriptorSetLayoutBinding uboV{};//*UBO
    uboV.binding = 0;
    uboV.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboV.descriptorCount = 1;//DescriptorCount refers to the number of elements that the binding has (1 for single elements)
    uboV.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;//This will make it accessible in Vertex Shader

    VkDescriptorSetLayoutBinding uboF{};//*UBO//*UBOF
    uboF.binding = 1;
    uboF.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboF.descriptorCount = 1;//DescriptorCount refers to the number of elements that the binding has (1 for single elements)
    uboF.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;//This will make it accessible in Fragment Shader

    //combined image sampler Layout binding //*TEX
    VkDescriptorSetLayoutBinding sampler{};
    sampler.binding = 2;
    sampler.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    sampler.descriptorCount = 1;
    sampler.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array<VkDescriptorSetLayoutBinding, 3> bindings{uboV, uboF, sampler};

    VkDescriptorSetLayoutCreateInfo layoutInfo{};//*SL
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();
    AVKDebug::assertVKSuccess(vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout), "create descriptor set layout");
}

/**
 * Specify the uniform buffer struct and create the uniform buffers.
 * Remember to allocate the memory from the VkDeviceMemory using vkAllocateMemory and bind the buffer to the memory using vkBindBufferMemory
 */
void AVKDescriptors::createUniformBuffers(/*this param should be used to set in a custom UBO object*/) {
    //resize the empty memory
    uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);//*UBO
    uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);//*UBO
    uniformBuffersF.resize(MAX_FRAMES_IN_FLIGHT);//*UBOF
    uniformBuffersMemoryF.resize(MAX_FRAMES_IN_FLIGHT);//*UBOF

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        //Remember host visible memory is needed to write to a buffer object?/image?
        createBuffer(sizeof(UniformBufferObjectV),//*UBO
                     VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     uniformBuffers[i],//*UBO
                     uniformBuffersMemory[i]);//*UBO
        //Secondary bind to a different buffer/memory location this is setup for fragment shader
        createBuffer(sizeof(UniformBufferObjectF),//*UBOF
                     VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     uniformBuffersF[i],//*UBOF
                     uniformBuffersMemoryF[i]);//*UBOF
    }
}

void AVKDescriptors::createDescriptorPool(std::vector<VkImage> swapChainImages) {//*MML
    uint32_t setCount = std::max<uint32_t>(static_cast<uint32_t>(swapChainImages.size()), MAX_FRAMES_IN_FLIGHT);

    std::array<VkDescriptorPoolSize, 2> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;//*UBO
    poolSizes[0].descriptorCount = 2 * setCount;//*SL
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;//*TEX
    poolSizes[1].descriptorCount = setCount;//*SL //Todo -- look into introducing multiple samplers, or use an alternative

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());//*SL
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = setCount;//*SL ,Todo -- Refer doc for why Frames in MAX_FRAMES_IN_FLIGHT is being used instead of swapChainImages.size()
    AVKDebug::assertVKSuccess(vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool), "create descriptor pool");
}

/*
 * Create VkDescriptorSets allocated from the VkDescriptorPool specified.
 * A VkDescriptorSet is a Vulkan object that represents a collection of descriptor resources.
 * Descriptor resources are used to provide shader inputs, such as uniform buffers, image samplers, and storage buffers.
 * To create the VkDescriptorSets, we will need to create a VkDescriptorPool.
 * - VkDescriptorSets are a pack of pointers that are bound together.
 * - Vulkan does not allow you to bind individual resources in shaders. They have to be grouped in the sets.
 * If you still insist on being able to bind them individually, then you will need a descriptor set for each resource.
 */
void AVKDescriptors::createDescriptorSets(std::vector<AVKTexture*> textures) { //*SL
    /*MAX_FRAMES_IN_FLIGHT is being used installed of swapChainImages.size()
      TODO -- some issue with incorrect size check function -> createSwapChain()*/
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool; //sets the desc_pool here with the pool info from createDescriptorPool()
    allocInfo.descriptorSetCount = static_cast<uint32_t>(layouts.size());
    allocInfo.pSetLayouts = layouts.data();

    descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    AVKDebug::assertVKSuccess(vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()), "allocate descriptor sets");

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo uboVInfo{};
        uboVInfo.buffer = uniformBuffers[i];
        uboVInfo.offset = 0;
        uboVInfo.range = sizeof(UniformBufferObjectV);//Todo -- range needs to be updated as per the uniform buffer object

        VkDescriptorBufferInfo uboFInfo{};
        uboFInfo.buffer = uniformBuffersF[i];
        uboFInfo.offset = 0;
        uboFInfo.range = sizeof(UniformBufferObjectF);//Todo -- range needs to be updated as per the uniform buffer object

        //combine image sampler *SL
        VkDescriptorImageInfo imageInfo{};//*TEX
        imageInfo.sampler = textures[0]->getVKSampler();
        imageInfo.imageView = textures[0]->getImageView();
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        std::array<VkWriteDescriptorSet, 3> descriptorWrites{};
        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = descriptorSets[i];
        descriptorWrites[0].dstBinding = 0;//*UBO
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].pBufferInfo = &uboVInfo;//*UBO

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = descriptorSets[i];
        descriptorWrites[1].dstBinding = 1;//*UBOF
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[1].pBufferInfo = &uboFInfo;//*UBOF

        descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[2].dstSet = descriptorSets[i];
        descriptorWrites[2].dstBinding = 2;//*TEX
        descriptorWrites[2].descriptorCount = 1;
        descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[2].pImageInfo = &imageInfo;//*TEX

        vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }
}

/** Clear descriptor resources */
void AVKDescriptors::clear() {
    for (size_t i = 0; i < uniformBuffers.size(); i++) {
        if (uniformBuffers[i] != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, uniformBuffers[i], nullptr);
            uniformBuffers[i] = VK_NULL_HANDLE;
        }
        if (uniformBuffersMemory[i] != VK_NULL_HANDLE) {
            vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
            uniformBuffersMemory[i] = VK_NULL_HANDLE;
        }
    }
    for (size_t i = 0; i < uniformBuffersF.size(); i++) {
        if (uniformBuffersF[i] != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, uniformBuffersF[i], nullptr);
            uniformBuffersF[i] = VK_NULL_HANDLE;
        }
        if (uniformBuffersMemoryF[i] != VK_NULL_HANDLE) {
            vkFreeMemory(device, uniformBuffersMemoryF[i], nullptr);
            uniformBuffersMemoryF[i] = VK_NULL_HANDLE;
        }
    }
    uniformBuffers.clear();
    uniformBuffersMemory.clear();
    uniformBuffersF.clear();
    uniformBuffersMemoryF.clear();
    descriptorSets.clear();

    if (descriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device, descriptorPool, nullptr);
        descriptorPool = VK_NULL_HANDLE;
    }
    if (descriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
        descriptorSetLayout = VK_NULL_HANDLE;
    }
}

/** Create buffer helper */
void AVKDescriptors::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) { //Make this helper
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    AVKDebug::assertVKSuccess(vkCreateBuffer(device, &bufferInfo, nullptr, &buffer), "create uniform buffer");

    VkMemoryRequirements memRequirements{};
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);
    AVKDebug::assertVKSuccess(vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory), "allocate uniform buffer memory");
    vkBindBufferMemory(device, buffer, bufferMemory, 0);
}

/** Helper function to find the correct memory type */
uint32_t AVKDescriptors::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties{};
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    throw std::runtime_error("no suitable memory type for descriptor buffer"); //Failed to find a suitable memory type!
}
