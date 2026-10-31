#pragma once

#include <cstdint>
#include <vector>
#include <vulkan/vulkan.h>

#include "vk_utils.h"
#include "vk_debughandler.h"
#include "vk_commandBuffers.h"
#include "vk_descriptors.h"
#include "vk_model.h"
#include "vk_shaderpipeline.h"
#include "vk_texture.h"

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities{};
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

class AurVK {
public:
    AurVK();
    ~AurVK();

    //Setup Vulkan Pipeline
    /** Initialize Vulkan */
    void initVulkan();
    /** Cleanup Vulkan resources */
    void cleanup(); //Shutdown

    /** Create Vulkan instance */
    void createInstance(); //Init
    /** Reset with new window and asset manager */
    void reset(HWND newWindow, AssetManager* newManager);

    //Todo -- Temp Public make getter setter replacements
    bool initialized = false;
    /** Render frame */
    void render();

    //--------------------------------------
private:
    /*INIT--*/
    //Setup Debugging and Validation
    /** Populate debug messenger create info */
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
    /** Setup debug messenger */
    void setupDebugMessenger(); //Init //optional

    //Vulkan Instance Creation
    /** Check validation layer support */
    bool checkValidationLayerSupport();//*VA
    /** Create surface */
    void createSurface(); //Init
    /** Pick physical device */
    void pickPhysicalDevice(); //Init
    /** Check if device is suitable */
    bool isDeviceSuitable(VkPhysicalDevice device);
    /** Find queue families */
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
    /** Check device extension support */
    bool checkDeviceExtensionSupport(VkPhysicalDevice device);
    /** Query swap chain support */
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
    /** Create logical device and queue */
    void createLogicalDeviceAndQueue(); //Init

    //Swap chain and Sync Objects
    /** Create sync objects */
    void createSyncObjects(); //Init
    /** Create swap chain */
    void createSwapChain(); //Init
    /** Recreate swap chain */
    void recreateSwapChain(); //Render
    /** Cleanup swap chain */
    void cleanupSwapChain(); //Cleanup

    //Create Render pass and Framebuffer
    /** Create image views */
    void createImageViews(); //Init
    /** Create render pass */
    void createRenderPass(); //Init
    /** Create framebuffers */
    void createFramebuffers(); //Init

    //Create DescriptorSet
    //void createDescriptorSetLayout();//Init//*MML

    //Create Shader and Pipeline
    /*Segregated*/
    /** Choose swap surface format */
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    /** Choose swap present mode */
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    /** Choose swap extent */
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

    //Transformations
    //Callbacks
    /** Update uniform buffer */
    void updateUniformBuffer(uint32_t currentImage);
    /*UPDATE--*/
    //-----------------------------------------------------------
    //Debugging
    // const bool enableValidationLayers = true; //This need to be triggered manually for debugging.
    // void assertVKSuccess(VkResult result, std::string message);
    /** Create debug utils messenger */
    VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
    /** Destroy debug utils messenger */
    void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
    /** Draw frame */
    void drawFrame();//Render
    /** Record command buffer */
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);//Render

    // At the time of writing, these five layers make up the VK_LAYER_LUNARG_standard_validation
    // meta-layer. According to presentation slides from LunarG, that isn't available in the Android
    // implementation, so this lists them out manually.
    // TODO -- only one seems to be working and the rest are not available in current device. Make sure to use checkValidationLayerSupport...
    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation",
        "VK_LAYER_GOOGLE_threading",
        "VK_LAYER_LUNARG_parameter_validation",
        "VK_LAYER_LUNARG_object_tracker",
        "VK_LAYER_LUNARG_core_validation",
        "VK_LAYER_GOOGLE_unique_objects"
    };

    const std::vector<const char*> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    bool enableValidationLayers = false;

    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
    std::vector<const char*> requiredExtensions;

    //Android File system context
    AssetManager* assetManager = nullptr;

    //Windowing and Instance creation
    HWND window = nullptr;

    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkInstance instance = VK_NULL_HANDLE;

    //Create Vulkan Instance and Device
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE; //Logical Device

    //Doc - https://stackoverflow.com/questions/55272626/what-is-actually-a-queue-family-in-vulkan
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    uint32_t currentFrame = 0; //uses the first frame where ever invoked in swapchain arrays.

    //Swap chain and Sync Objects
    VkSwapchainKHR swapChain = VK_NULL_HANDLE;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat{};
    VkExtent2D swapChainExtent{};
    std::vector<VkImageView> swapChainImageViews;
    //Create Render pass and Framebuffer //TODO -- look into render to texture here
    VkRenderPass renderPass = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> swapChainFramebuffers;

    //Create Descriptor Sets
    /*Segregated*/
    AVKDescriptors* aDescriptor = nullptr; //*MML
    AVKDescriptors* aDescriptor2 = nullptr; //*MML

    //Create Shader and Pipeline
    /*Segregated*/
    AVKShaderPipeline* aPipeline = nullptr; //*MML
    AVKShaderPipeline* aPipeline2 = nullptr; //*MML

    //Command Buffer create, record and Draw
    /*Segregated*/
    AVKCommandBuffer* aCommandBuffer = nullptr; //*MML
    // AVKCommandBuffer* aCommandBuffer2; //*MML

    //Texturing
    /*Segregate texturing Moved to VKTexture*/
    AVKTexture* aTexture = nullptr;
    AVKTexture* aTexture2 = nullptr;

    //Mesh Loading /*MML
    /*Need to rebuild this class to handle model loading*/
    AVKMesh* aMesh1 = nullptr;
    AVKMesh* aMesh2 = nullptr;
    // std::vector<AVKMesh*> aMeshes;

    //MIS events
    bool orientationChange = false;
};
