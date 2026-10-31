#pragma once

#include <string>
#include <vector>
#include <vulkan/vulkan.h>

#include "vk_utils.h"

class AVKShaderPipeline {
public:
    /** Constructor */
    AVKShaderPipeline(VkDevice device, VkRenderPass renderPass, VkDescriptorSetLayout descriptorSetLayout, AssetManager* assetManager);
    /** Destructor */
    ~AVKShaderPipeline();

    /** Create graphics pipeline */
    void createGraphicsPipeline(std::string vertPath, std::string fragPath);//*MML
    /** Clear VK objects */
    void clear();

    /** Get pipeline */
    VkPipeline getPipeline() const { return graphicsPipeline; }
    /** Get pipeline layout */
    VkPipelineLayout getPipelineLayout() const { return pipelineLayout; }

private:
    //Externals
    VkDevice device;
    VkRenderPass renderPass;
    VkDescriptorSetLayout descriptorSetLayout;//*MML
    AssetManager* assetManager;

    //Internals
    VkPipeline graphicsPipeline = VK_NULL_HANDLE;//*MML
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;//*MML //Used inside recordCommandBuffer() //*local*/

    /** Create shader module */
    VkShaderModule createShaderModule(const std::vector<char>& code);
};
