#include "vk_model.h"

#include <cstring>
#include <glm/gtc/matrix_transform.hpp>

#include "vk_debughandler.h"

/** Destructor */
AVKMesh::~AVKMesh() {
    cleanup();
}

void AVKMesh::loadMeshData(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue graphicsQueue) {
    this->device = device;
    this->physicalDevice = physicalDevice;
    this->commandPool = commandPool;
    this->graphicsQueue = graphicsQueue;

    loadModel();//Todo -- this needs to be replaced with an external obj or gltf loading functionality.
    createVertexBuffer();
    createIndexBuffer();
    setModelMatrix();
}

/** Cleanup mesh resources */
void AVKMesh::cleanup() {
    if (vertexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, vertexBuffer, nullptr);
        vertexBuffer = VK_NULL_HANDLE;
    }
    if (vertexBufferMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, vertexBufferMemory, nullptr);
        vertexBufferMemory = VK_NULL_HANDLE;
    }
    if (indexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, indexBuffer, nullptr);
        indexBuffer = VK_NULL_HANDLE;
    }
    if (indexBufferMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, indexBufferMemory, nullptr);
        indexBufferMemory = VK_NULL_HANDLE;
    }
}

void AVKMesh::render(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout) {
    /*Update the push Constant uniform in vertex Shader*/
    updatePushConstant(commandBuffer, pipelineLayout);//set the model matrix here //*PUC
    /*Update the primitive data buffer handles to be staged for rendering*/
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, offsets);
    vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    /*Calling Render/Draw of Model here*/
    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
}

/** Set position */
void AVKMesh::setPosition(glm::vec3 value) {
    position = value;
    setModelMatrix();
}

/** Set rotation */
void AVKMesh::setRotation(glm::vec3 value) {
    rotation = value;
    setModelMatrix();
}

/** Set scale */
void AVKMesh::setScale(glm::vec3 value) {
    scale = value;
    setModelMatrix();
}

/**
 * Sets the values matrices for the model matrix
 * works in implementing translation, rotation and scaling
 * Used by: the glwidget class initialise() or paintGL()
 */
void AVKMesh::setModelMatrix() {
    translationMatrix = glm::translate(glm::mat4(1.0f), position);
    rotationMatrix = glm::rotate(glm::mat4(1.0f), rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
    rotationMatrix = glm::rotate(rotationMatrix, rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
    rotationMatrix = glm::rotate(rotationMatrix, rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
    scalingMatrix = glm::scale(glm::mat4(1.0f), scale);
    modelMatrix = translationMatrix * rotationMatrix * scalingMatrix;
}

/** Update push constant with model matrix */
void AVKMesh::updatePushConstant(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout) {
    vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &modelMatrix);
}

//
//  __  __           _       _            ____                 _
// |  \/  | ___   __| |_   _| | ___ _ __ / ___|_ __ __ _  __| | ___
// | |\/| |/ _ \ / _` | | | | |/ _ \ '__| |  _| '__/ _` |/ _` |/ _ \
// | |  | | (_) | (_| | |_| | |  __/ |  | |_| | | | (_| | (_| |  __/
// |_|  |_|\___/ \__,_|\__,_|_|\___|_|   \____|_|  \__,_|\__,_|\___|
//
void AVKMesh::loadModel() {
    //Set transforms to default state for model at load
    this->scale = glm::vec3(1.0f);
    this->position = glm::vec3(0.0f);
    this->rotation = glm::vec3(0.0f);
    /**
     * Base 2D Quad
     * This is the default render-able entity that gets set
     * so initialisation of the Entity object
     * if no other primitive info is passed to the renderer.
     */
    //Vertices
    std::vector<float> vert = {
        //Positions
        0.5f, 0.5f, 0.0f,// top right
        0.5f, -0.5f, 0.0f,// bottom right
        -0.5f, -0.5f, 0.0f,// bottom left
        -0.5f, 0.5f, 0.0f // top left
    };
    //Indices
    std::vector<int> ind = {
        0, 1, 2,
        0, 2, 3,
    }; //Triangle order
    /**
     * Coordinates from 0, 0 in the top-left corner to 1, 1 in the bottom-right corner.
     * Feel free to experiment with different coordinates.
     * Try using coordinates below 0 or above 1 to see the addressing modes in action!
     *
     * The OBJ format assumes a coordinate system where a vertical coordinate of 0 means the bottom of the image,
     * however we've uploaded our image into Vulkan in a top to bottom orientation where 0 means the top of the image.
     * solve this by flipping the vertical component of the texture coordinates:
     */
    //*ORF
    /*(0,0)TR-------------------TL(1,0)//
     *
     *
     *
     *
     *
     *
     *(0,1)BR-------------------BT(1,1)//
     */
    //UV mapping for VK is corresponding Quad is not flipped or CCW correction is not made for orientation matching.
    std::vector<float> texCoord = {
        0.0, 0.0,// top right
        0.0, 1.0,// bottom right
        1.0, 1.0 // bottom left
    };
    vertices = {
        {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
        {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
        {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
        {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}}
    };
    indices = {0, 1, 2, 2, 3, 0};
}

/*
 * Buffers in Vulkan are regions of memory used for storing arbitrary data that can be read by the graphics card.
 * They can be used to store vertex data, which we'll do here, but they can also be used for many other purposes.
 * Unlike the Vulkan objects we've been dealing with so far, buffers do not automatically allocate memory for themselves.
 * The work from the previous chapters has already shown that the Vulkan API puts the programmer in control of almost everything and memory management is one of those things.
 */
void AVKMesh::createVertexBuffer() { //These are called vertex buffers , that holds the vertex data for the mesh
    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingBufferMemory = VK_NULL_HANDLE;
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingBuffer, stagingBufferMemory);

    void* data = nullptr;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    std::memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
    vkUnmapMemory(device, stagingBufferMemory);

    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);
    copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void AVKMesh::createIndexBuffer() {
    VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingBufferMemory = VK_NULL_HANDLE;
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingBuffer, stagingBufferMemory);

    void* data = nullptr;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    std::memcpy(data, indices.data(), static_cast<size_t>(bufferSize));
    vkUnmapMemory(device, stagingBufferMemory);

    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);
    copyBuffer(stagingBuffer, indexBuffer, bufferSize);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

/** Begin single time commands */
VkCommandBuffer AVKMesh::beginSingleTimeCommands() {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    AVKDebug::assertVKSuccess(vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer), "allocate mesh command buffer");

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    AVKDebug::assertVKSuccess(vkBeginCommandBuffer(commandBuffer, &beginInfo), "begin mesh command buffer");
    return commandBuffer;
}

/** End single time commands */
void AVKMesh::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
    AVKDebug::assertVKSuccess(vkEndCommandBuffer(commandBuffer), "end mesh command buffer");

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    AVKDebug::assertVKSuccess(vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE), "submit mesh command buffer");
    vkQueueWaitIdle(graphicsQueue);
    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

/** Copy buffer */
void AVKMesh::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkBufferCopy copyRegion{};
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    endSingleTimeCommands(commandBuffer);
}

/** Create buffer */
void AVKMesh::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    AVKDebug::assertVKSuccess(vkCreateBuffer(device, &bufferInfo, nullptr, &buffer), "create mesh buffer");

    VkMemoryRequirements memRequirements{};
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);
    AVKDebug::assertVKSuccess(vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory), "allocate mesh buffer memory");
    vkBindBufferMemory(device, buffer, bufferMemory, 0);
}

/** Find memory type */
uint32_t AVKMesh::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties{};
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    throw std::runtime_error("no suitable memory type for mesh");
}
