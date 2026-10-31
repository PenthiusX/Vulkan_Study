#include "vk_shaderpipeline.h"

#include <array>
#include <glm/glm.hpp>

#include "vk_debughandler.h"
#include "vk_vertex.h"

/** Constructor */
AVKShaderPipeline::AVKShaderPipeline(VkDevice device, VkRenderPass renderPass, VkDescriptorSetLayout descriptorSetLayout, AssetManager* assetManager)
    : device(device), renderPass(renderPass), descriptorSetLayout(descriptorSetLayout), assetManager(assetManager) {
}

/** Destructor */
AVKShaderPipeline::~AVKShaderPipeline() {
    clear();
}

/**
 * Creates a graphics pipeline loading a simple vertex and fragment shader, both with 'main' set as entrypoint. A list of standard parameters:
 * - The vertex input coming from the application is set to empty - we are hardcoding the triangle in the vertex shader. A list of standard parameters:
 * - The input assembly is configured to draw triangle lists
 * - We intend to draw onto the whole screen, so the scissoring extent is specified as being the whole swap-chain extent.
 * - The rasterizer is set to discard fragments beyond the near and far planes (depthClampEnable=false) as well as sending geometry to the...
 * We consider geometry in terms of the clockwise order of their respective vertex input.
 * - Multisampling is disabled
 * - Depth and stencil testing are disabled
 * ColorBlending is set to opaque mode, meaning any new fragments will
 * overwrite the ones already existing in the framebuffer
 * - We utilise Vulkan's concept of dynamic state for viewport and scissoring.
 * - The other option is to hardcode the viewport/scissor options, however this means needing to recreate the whole graphics pipeline...
 * - The pipeline layout sends 1 uniform buffer object to the shader containing a 4x4 rotation matrix specified by the descriptorSetLayout.
 * This is required in order to render a rotated scene when the device has been rotated.
 */
/**
 * @param vertexShaderAssetPath
 * @param fragmentShaderAssetPath
 */
void AVKShaderPipeline::createGraphicsPipeline(std::string vertPath, std::string fragPath) { //*MML
    std::vector<char> vertCode = readFile(vertPath.c_str(), assetManager);
    std::vector<char> fragCode = readFile(fragPath.c_str(), assetManager);
    VkShaderModule vertModule = createShaderModule(vertCode);//Todo -- Look into the compilation process.
    VkShaderModule fragModule = createShaderModule(fragCode);

    VkPipelineShaderStageCreateInfo vertStage{};
    vertStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertStage.module = vertModule;
    vertStage.pName = "main";

    VkPipelineShaderStageCreateInfo fragStage{};
    fragStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragStage.module = fragModule;
    fragStage.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertStage, fragStage};

    VkVertexInputBindingDescription bindingDescription = Vertex::getBindingDescription();//*EML
    std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions = Vertex::getAttributeDescriptions();//*EML

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;//*EML changed this from 0 to 1
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;//*EML replace nullptr
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());//*EML replace 0
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();//*EML replace nullptr

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE;//*ORF
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;//Winding order //*ORF
    rasterizer.depthBiasEnable = VK_FALSE;//Depth testing

    VkPipelineMultisampleStateCreateInfo multisampling{};//Todo -- look into probably accelerated multisampling
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    //-----
    //VkPipelineDepthStencilStateCreateInfo depthStencil{};//Depth testing
    //-----
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE; //Change for enable diable blending

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    std::array<VkDynamicState, 2> dynamicStates{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    /* Push constants let us send a small amount of data (it has a limited size) to the shader, in a very simple and performant way.
     * Push constants can send data to any shader stage (both vertex and fragment shaders), and are stored in the command buffer itself...
     */
    VkPushConstantRange pushConstantRange{};//Added//*PUC
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(glm::mat4);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
    //Adding push constants to be available for use using vkCmdPushConstants();
    pipelineLayoutInfo.pushConstantRangeCount = 1;//changed 0 to 1//*PUC
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;//changed from nullptr//*PUC
    AVKDebug::assertVKSuccess(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout), "create pipeline layout");

    /*Use the setting above to define the pipeline info below*/
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = nullptr;//Todo
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    AVKDebug::assertVKSuccess(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline), "create graphics pipeline");

    vkDestroyShaderModule(device, fragModule, nullptr);
    vkDestroyShaderModule(device, vertModule, nullptr);
}

void AVKShaderPipeline::clear() {
    if (graphicsPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(device, graphicsPipeline, nullptr);
        graphicsPipeline = VK_NULL_HANDLE;
    }
    if (pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        pipelineLayout = VK_NULL_HANDLE;
    }
}

/** Create shader module */
VkShaderModule AVKShaderPipeline::createShaderModule(const std::vector<char>& code) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
    VkShaderModule shaderModule = VK_NULL_HANDLE;
    AVKDebug::assertVKSuccess(vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule), "create shader module");
    return shaderModule;
}
