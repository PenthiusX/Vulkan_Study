#pragma once

#include <cstdint>
#include <vector>
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#include "vk_vertex.h"

class AVKMesh {
public:
    AVKMesh() = default;
    ~AVKMesh();

    /** Load mesh data and create buffers */
    void loadMeshData(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue graphicsQueue);
    /** Cleanup mesh resources */
    void cleanup();
    /** Render the mesh */
    void render(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout);

    //Transforms
    /** Set position */
    void setPosition(glm::vec3 value);
    /** Set rotation */
    void setRotation(glm::vec3 value);
    /** Set scale */
    void setScale(glm::vec3 value);
    glm::vec3 getPosition() const { return position; }
    glm::vec3 getRotation() const { return rotation; }
    glm::vec3 getScale() const { return scale; }

private:
    //Transforms
    glm::vec3 position{0.0f, 0.0f, 0.0f};
    glm::vec3 rotation{0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f, 1.0f, 1.0f};
    glm::mat4 modelMatrix{1.0f}; //Final matrix after all transformations are applied
    glm::mat4 translationMatrix{1.0f};
    glm::mat4 rotationMatrix{1.0f};
    glm::mat4 scalingMatrix{1.0f};

    //--------------------------------------
    //Model Transformation
    /** Set model matrix from transforms */
    void setModelMatrix();
    /** Update push constant with model matrix */
    void updatePushConstant(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout);
    //--------------------------------------

    //VK context data
    VkDevice device = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;

    //Model Data
    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
    VkBuffer indexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    //Model Loading
    /** Load model vertices and indices */
    void loadModel();
    /** Begin single time commands */
    VkCommandBuffer beginSingleTimeCommands();
    /** End single time commands */
    void endSingleTimeCommands(VkCommandBuffer commandBuffer);
    /** Copy buffer */
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    /** Find memory type */
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    /** Create buffer */
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
    /** Create vertex buffer */
    void createVertexBuffer();
    /** Create index buffer */
    void createIndexBuffer();
};
