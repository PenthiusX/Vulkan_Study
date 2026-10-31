#include "vk_renderer.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <set>
#include <stdexcept>

#include <glm/gtc/matrix_transform.hpp>

/** Constructor */
AurVK::AurVK() = default;

/** Destructor */
AurVK::~AurVK() {
    cleanup();
}

//-----------------------------------
/**
 * Initialises all preprocesses setup operations for VK instance creation
 */
void AurVK::initVulkan() {
    //Device Initialisation stack
    createInstance();//Instance Creation.
    checkValidationLayerSupport();//Optional Validation layer check.
    createSurface();//Create the Surface / windowing instance
    pickPhysicalDevice();//Create Physical Device.
    createLogicalDeviceAndQueue();//Create the Device for use.
    //Debug~~~~~~~~~~~~~~~~
    setupDebugMessenger();
    //~~~~~~~~~~~~~~~~~~~~~~~~
    //Realtime Rendering Frames to screen
    createSwapChain();
    createImageViews();
    //---
    createRenderPass();
    //-----------------------------------
    /*Segregated*/
    aDescriptor = new AVKDescriptors(device, physicalDevice);
    aDescriptor->createDescriptorSetLayout();//*MML

    aDescriptor2 = new AVKDescriptors(device, physicalDevice);
    aDescriptor2->createDescriptorSetLayout();//*MML
    //-----------------------
    //Create Shader and Pipeline
    /*Segregated*/
    aPipeline = new AVKShaderPipeline(device, renderPass, aDescriptor->getDescriptorSetLayout(), assetManager);
    aPipeline->createGraphicsPipeline("shaders/shader.vert.spv", "shaders/shader.frag.spv");//*MML

    aPipeline2 = new AVKShaderPipeline(device, renderPass, aDescriptor2->getDescriptorSetLayout(), assetManager);
    aPipeline2->createGraphicsPipeline("shaders/shader.vert.spv", "shaders/shader.frag.spv");//*MML
    //-----------------------
    /*Segregated*/
    aCommandBuffer = new AVKCommandBuffer(device, physicalDevice, surface);
    aCommandBuffer->createCommandPool();
//        this->aCommandBuffer2 = new AVKCommandBuffer(this->device, this->physicalDevice, this->surface);
//        this->aCommandBuffer2->createCommandPool();
    //-----------------------
    createFramebuffers();
    //-----------------------
    //Texturing
    /*Segregated*/
    aTexture = new AVKTexture(device, physicalDevice, aCommandBuffer->getCommandPool(), graphicsQueue, assetManager);
    std::vector<uint8_t> data = LoadBinaryFileToVector("texture1.png", assetManager);
    aTexture->loadTexture(data); //*B64
//        this->aTexture->loadTexture("texture1.png");//*TEX

    aTexture2 = new AVKTexture(device, physicalDevice, aCommandBuffer->getCommandPool(), graphicsQueue, assetManager);
    aTexture2->loadTexture("texture.png");//*TEX

    std::vector<AVKTexture*> textures;
    textures.push_back(aTexture);
    textures.push_back(aTexture2);

    //-----------------------
    // Mesh Loading
    /*Segregated for Entity structure WIP*/
    aMesh1 = new AVKMesh();
    aMesh2 = new AVKMesh();
    aMesh1->loadMeshData(device, physicalDevice, aCommandBuffer->getCommandPool(), graphicsQueue); //*MML
    aMesh2->loadMeshData(device, physicalDevice, aCommandBuffer->getCommandPool(), graphicsQueue); //*MML
//        auto m = new AVKMesh();
//        m->loadMeshData(this->device, this->physicalDevice, this->aCommandBuffer->getCommandPool(), this->graphicsQueue); //*MML
//        aMeshes.push_back(m);

    aMesh1->setPosition({-0.55f, 0.0f, 0.0f});
    aMesh1->setScale({0.8f, 0.8f, 1.0f});
    aMesh2->setPosition({0.55f, 0.0f, 0.0f});
    aMesh2->setScale({0.8f, 0.8f, 1.0f});

    //-----------------------
    /*Segregated*/
    aDescriptor->createUniformBuffers();
    aDescriptor->createDescriptorPool(swapChainImages);
    aDescriptor->createDescriptorSets(textures); //*MML

    aDescriptor2->createUniformBuffers();
    aDescriptor2->createDescriptorPool(swapChainImages);
    aDescriptor2->createDescriptorSets(textures); //*MML
    //-----------------------
    /*Segregated*/
    aCommandBuffer->createCommandBuffers();
//        this->aCommandBuffer2->createCommandBuffers();
    //-----------------------
    createSyncObjects();
    //-----------------------
    initialized = true;//This flag track completion of initialisation
}

/** Reset with new window and asset manager */
void AurVK::reset(HWND newWindow, AssetManager* newManager) {
    window = newWindow;
    assetManager = newManager;

    if (!initialized) {
        initVulkan();
        return;
    }

    vkDeviceWaitIdle(device);
    cleanupSwapChain();
    vkDestroySurfaceKHR(instance, surface, nullptr);
    createSurface();
    createSwapChain();
    createImageViews();
    createFramebuffers();
}

/** Render frame */
void AurVK::render() {
    drawFrame();
}

/*---------------------------------------------------------------------------------------------
██╗   ██╗██╗  ██╗
██║   ██║██║ ██╔╝
██║   ██║█████╔╝
╚██╗ ██╔╝██╔═██╗
 ╚████╔╝ ██║  ██╗
  ╚═══╝  ╚═╝  ╚═╝

██╗███╗   ██╗███████╗████████╗ █████╗ ███╗   ██╗ ██████╗███████╗
██║████╗  ██║██╔════╝╚══██╔══╝██╔══██╗████╗  ██║██╔════╝██╔════╝
██║██╔██╗ ██║███████╗   ██║   ███████║██╔██╗ ██║██║     █████╗
██║██║╚██╗██║╚════██║   ██║   ██╔══██║██║╚██╗██║██║     ██╔══╝
██║██║ ╚████║███████║   ██║   ██║  ██║██║ ╚████║╚██████╗███████╗
╚═╝╚═╝  ╚═══╝╚══════╝   ╚═╝   ╚═╝  ╚═╝╚═╝  ╚═══╝ ╚═════╝╚══════╝
---------------------------------------------------------------------------------------------*/
/**
 * The VkInstance object represents an application's instance of the Vulkan runtime.
 * It is the root object of the Vulkan API and is used to retrieve information about and instantiate Vulkan device objects and any layers.
 *
 * When an application creates a VkInstance, it must provide information about itself, such as its name, version, and the Vulkan instance extensions it requires.
 *
 * The Vulkan API design includes a layer system that provides a mechanism for intercepting and processing API calls before they reach the driver.
 * The application can designate layers to activate when creating a VkInstance.
 * The most commonly used layer is the Vulkan validation layer, which provides runtime analysis of API usage for errors or suboptimal performance.
 *
 * Once a VkInstance has been created, the application can use it to query for the available physical devices on the system, create logical devices, etc.
 *
 * A VkInstance is typically created once at the start of the application and destroyed at the end.
 * However it is possible to create multiple VkInstances within the same application.
 */
void AurVK::createInstance() {
    enableValidationLayers = checkValidationLayerSupport();//validation layers requested, but not available!//*VA
    if (!enableValidationLayers) {
        LOGI("Validation layers unavailable; running without the debug messenger.");
    }

    uint32_t availableExtensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(availableExtensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionCount, availableExtensions.data());

    auto isExtensionAvailable = [&availableExtensions](const char* name) {
        for (const VkExtensionProperties& extension : availableExtensions) {
            if (std::strcmp(extension.extensionName, name) == 0) {
                return true;
            }
        }
        return false;
    };

    requiredExtensions = AVKDebug::getRequiredInstanceExtensions();
    std::vector<const char*> enabledExtensions;
    for (const char* extension : requiredExtensions) {
        if (isExtensionAvailable(extension)) {
            enabledExtensions.push_back(extension);
        }
    }
    if (!isExtensionAvailable(VK_EXT_DEBUG_UTILS_EXTENSION_NAME)) {
        enableValidationLayers = false;
    }

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Aura";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Aura";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size());
    createInfo.ppEnabledExtensionNames = enabledExtensions.data();

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
        populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = &debugCreateInfo;
    }

    AVKDebug::assertVKSuccess(vkCreateInstance(&createInfo, nullptr, &instance), "create Vulkan instance");
    LOGI("Vulkan instance created.");
}

/**
 * A VkSurfaceKHR object represents a surface that can be the target of rendering operations.
 * Create a surface using a reference to the application window object.
 * To display graphics on the device screen, the application can use it to create a VkSwapchainKHR object.
 */
void AurVK::createSurface() {
    //Todo -- current error in getting the right windowing context.
    VkWin32SurfaceCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    createInfo.hwnd = window;
    createInfo.hinstance = GetModuleHandle(nullptr);
    AVKDebug::assertVKSuccess(vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, &surface), "create Win32 surface");
}

bool AurVK::checkValidationLayerSupport() {//*VA
    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : validationLayers) {
        bool layerFound = false;
        for (const VkLayerProperties& layerProperties : availableLayers) {
            if (std::strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }
        if (!layerFound) {
            return false;
        }
    }
    return true;
}

/** Find queue families */
QueueFamilyIndices AurVK::findQueueFamilies(VkPhysicalDevice device) {
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

/** Check if device is suitable */
bool AurVK::isDeviceSuitable(VkPhysicalDevice device) {
    QueueFamilyIndices indices = findQueueFamilies(device);
    bool extensionsSupported = checkDeviceExtensionSupport(device);

    bool swapChainAdequate = false;
    if (extensionsSupported) {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }
    return indices.isComplete() && extensionsSupported && swapChainAdequate;
}

/** Check device extension support */
bool AurVK::checkDeviceExtensionSupport(VkPhysicalDevice device) {
    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> required(deviceExtensions.begin(), deviceExtensions.end());
    for (const VkExtensionProperties& extension : availableExtensions) {
        required.erase(extension.extensionName);
    }
    return required.empty();
}

/** Pick physical device */
void AurVK::pickPhysicalDevice() {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    if (deviceCount == 0) {
        throw std::runtime_error("no Vulkan-capable physical device found");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    //set the physical device here attached GPUs that need to be used to render;
    //should run first.
    for (VkPhysicalDevice candidate : devices) {
        if (isDeviceSuitable(candidate)) {
            physicalDevice = candidate;
            break;
        }
    }
    if (physicalDevice == VK_NULL_HANDLE) {
        //failed to find a suitable GPU!
        throw std::runtime_error("no suitable Vulkan physical device found");
    }

    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(physicalDevice, &properties);
    LOGI("Using GPU: %s", properties.deviceName);
}

/** Query swap chain support */
SwapChainSupportDetails AurVK::querySwapChainSupport(VkPhysicalDevice device) {
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
    }
    return details;
}

/** Choose swap surface format */
VkSurfaceFormatKHR AurVK::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    for (const VkSurfaceFormatKHR& format : availableFormats) {
        if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return format;
        }
    }
    return availableFormats[0];
}

/** Choose swap present mode */
VkPresentModeKHR AurVK::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
    //Todo -- what is the value
    return VK_PRESENT_MODE_FIFO_KHR; //this is a glfw tie immediate callback , replace this with the Android surface mess...
}

/** Choose swap extent */
VkExtent2D AurVK::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;//Gets the screen resolution here , Todo -- look into using this to set the framebuffer resolution
    }
    // Else will not be used in most cases if the window is available.
    //if the platform allows that , set the framebuffer size manually.
    RECT clientRect{};
    GetClientRect(window, &clientRect);
    VkExtent2D actualExtent = {
        static_cast<uint32_t>(clientRect.right - clientRect.left),
        static_cast<uint32_t>(clientRect.bottom - clientRect.top)
    };
    actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
    return actualExtent;
}

/**
 * Creates the main Device handle for use in the application
 * A VkDevice is a Vulkan object that represents a logical Vulkan device.
 * A VkDevice is created from a VkPhysicalDevice, and is specific to the application that created it.
 * It has its own state and resources, and it is independent of other logical devices that may be created from the same physical device.
 */
void AurVK::createLogicalDeviceAndQueue() {
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<int> uniqueQueueFamilies = {indices.graphicsFamily, indices.presentFamily};
    float queuePriority = 1.0f;
    for (int queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = static_cast<uint32_t>(queueFamily);
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};
    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();
    if (enableValidationLayers) {//*VA
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    }

    AVKDebug::assertVKSuccess(vkCreateDevice(physicalDevice, &createInfo, nullptr, &device), "create logical device");
    vkGetDeviceQueue(device, static_cast<uint32_t>(indices.graphicsFamily), 0, &graphicsQueue);
    vkGetDeviceQueue(device, static_cast<uint32_t>(indices.presentFamily), 0, &presentQueue);
}

//-----------------------------------
/**
 * Create a swapchain
 * Vulkan does not have the concept of a "default framebuffer", hence it requires an infrastructure that will own the buffers we will render to before we visualize them on the screen.
 * This infrastructure is known as the swap chain and must be created explicitly in Vulkan. The swap chain is essentially a queue of images that are waiting to be presented to the screen.
 * Our application will acquire such an image to draw to it, and then return it to the queue. How exactly the queue works and the conditions for presenting an image from the queue depend on how the swap chain is set up.
 * but the general purpose of the swap chain is to synchronize the presentation of images with the refresh rate of the screen.
 */
void AurVK::createSwapChain() {
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    //Todo -- look into this
    //https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPresentModeKHR.html
    //for a discussion of the different present modes.
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);//Todo -- this is where the the framebuffer res is set like GLViewport

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
    uint32_t queueFamilyIndices[] = {
        static_cast<uint32_t>(indices.graphicsFamily),
        static_cast<uint32_t>(indices.presentFamily)
    };
    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    AVKDebug::assertVKSuccess(vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain), "create swapchain");

    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;
}

/* -------------------------------------------------------------
 * !NOTE! Implementations that are required be recreated for different scene object/entities will need to be custom configured for
 * unique visualisation demands and use cases. The initial segregation for these task have been moved to the
 * Segregated Classes are vk_descriptor , vk_model , vk_shaderpipeline , vk_texture , vk_commandBuffer . Todo -- swap chain , device and render
 * Sets of unique vk_shaderpipeline,vk_descriptor(and include texture) needs to be created and record/bound before the draw of the entity
 * See Implementation in Record CommandBuffer.
 */
void AurVK::recreateSwapChain() {
    vkDeviceWaitIdle(device);
    cleanupSwapChain();
    createSwapChain();
    createImageViews();
    createFramebuffers();
}

void AurVK::cleanupSwapChain() {
    for (VkFramebuffer framebuffer : swapChainFramebuffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }
    swapChainFramebuffers.clear();

    for (VkImageView imageView : swapChainImageViews) {
        vkDestroyImageView(device, imageView, nullptr);
    }
    swapChainImageViews.clear();

    if (swapChain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(device, swapChain, nullptr);
        swapChain = VK_NULL_HANDLE;
    }
    swapChainImages.clear();
}

//-----------------------
/*---------------------------------------------------------------------------------------------
██████╗ ███████╗███╗   ██╗██████╗ ███████╗██████╗
██╔══██╗██╔════╝████╗  ██║██╔══██╗██╔════╝██╔══██╗
██████╔╝█████╗  ██╔██╗ ██║██║  ██║█████╗  ██████╔╝
██╔══██╗██╔══╝  ██║╚██╗██║██║  ██║██╔══╝  ██╔══██╗
██║  ██║███████╗██║ ╚████║██████╔╝███████╗██║  ██║
╚═╝  ╚═╝╚══════╝╚═╝  ╚═══╝╚═════╝ ╚══════╝╚═╝  ╚═╝

██████╗  █████╗ ███████╗███████╗     █████╗ ███╗   ██╗██████╗
██╔══██╗██╔══██╗██╔════╝██╔════╝    ██╔══██╗████╗  ██║██╔══██╗
██████╔╝███████║███████╗███████╗    ███████║██╔██╗ ██║██║  ██║
██╔═══╝ ██╔══██║╚════██║╚════██║    ██╔══██║██║╚██╗██║██║  ██║
██║     ██║  ██║███████║███████║    ██║  ██║██║ ╚████║██████╔╝
╚═╝     ╚═╝  ╚═╝╚══════╝╚══════╝    ╚═╝  ╚═╝╚═╝  ╚═══╝╚═════╝

███████╗██████╗  █████╗ ███╗   ███╗███████╗██████╗ ██╗   ██╗███████╗███████╗███████╗██████╗
██╔════╝██╔══██╗██╔══██╗████╗ ████║██╔════╝██╔══██╗██║   ██║██╔════╝██╔════╝██╔════╝██╔══██╗
█████╗  ██████╔╝███████║██╔████╔██║█████╗  ██████╔╝██║   ██║█████╗  █████╗  █████╗  ██████╔╝
██╔══╝  ██╔══██╗██╔══██║██║╚██╔╝██║██╔══╝  ██╔══██╗██║   ██║██╔══╝  ██╔══╝  ██╔══╝  ██╔══██╗
██║     ██║  ██║██║  ██║██║ ╚═╝ ██║███████╗██████╔╝╚██████╔╝██║     ██║     ███████╗██║  ██║
╚═╝     ╚═╝  ╚═╝╚═╝  ╚═╝╚═╝     ╚═╝╚══════╝╚═════╝  ╚═════╝ ╚═╝     ╚═╝     ╚══════╝╚═╝  ╚═╝
---------------------------------------------------------------------------------------------*/
/*
 * Create Render pass and Framebuffer
 *
 * A VkImageView is a Vulkan object that describes how to access a VkImage.
 * It specifies the subresource range of the image to access, the pixel format to use, and the swizzle to apply to the channels.
 *
 * A VkRenderPass is a Vulkan object that describes how the GPU should render a scene.
 * It specifies the attachments that will be used, the order in which they will be rendered to, and how they will be used at each stage of the render pass.
 *
 * A VkFramebuffer is a Vulkan object that represents a set of image views that will be used as attachments during execution of a render pass.
 * In other words, it binds the actual image attachments to the render pass.
 */
void AurVK::createImageViews() {
    swapChainImageViews.resize(swapChainImages.size());

    for (size_t i = 0; i < swapChainImages.size(); i++) {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = swapChainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = swapChainImageFormat;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;
        AVKDebug::assertVKSuccess(vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]), "create swapchain image view");
    }
}

/**
 * Attachment in Vulkan is what is usually known as render target, which is usually an image used as output for rendering.
 * Only the format needs to be described here, for example , the render pass may output a specific color format or depth/stencil format.
 * You also need to specify whether your attachment should have its content preserved, discarded, or cleared at the beginning of the render pass.
 */
void AurVK::createRenderPass() {
    //color attachment configuration
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapChainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;
    AVKDebug::assertVKSuccess(vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass), "create render pass");
}

/**
 * Link to actual images that can be used for attachments (render targets).
 * A VkFramebuffer represents all the attachments needed for a render pass and the set of image views.
 * Create a Framebuffer object by specifying the render pass and the set of image views.
 */
void AurVK::createFramebuffers() {
    swapChainFramebuffers.resize(swapChainImageViews.size());

    for (size_t i = 0; i < swapChainImageViews.size(); i++) {
        VkImageView attachments[] = {swapChainImageViews[i]};

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = swapChainExtent.width;
        framebufferInfo.height = swapChainExtent.height;
        framebufferInfo.layers = 1;
        AVKDebug::assertVKSuccess(vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]), "create framebuffer");
    }
}

/** Create sync objects */
void AurVK::createSyncObjects() {
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        AVKDebug::assertVKSuccess(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]), "create image available semaphore");
        AVKDebug::assertVKSuccess(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]), "create render finished semaphore");
        AVKDebug::assertVKSuccess(vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]), "create in-flight fence");
    }
}

/**
 * Runs in Create info as a backup debugging enablement.
 */
void AurVK::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
    AVKDebug::populateDebugMessengerCreateInfo(createInfo);
}

/**
 * Enable verbose debugging, is enabled by setting enable validation layers.
 */
void AurVK::setupDebugMessenger() { //*VA
    if (!enableValidationLayers) {
        return;
    }

    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    populateDebugMessengerCreateInfo(createInfo);
    AVKDebug::assertVKSuccess(CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger), "setup debug messenger");
}

/**
 * Return
 */
/** Create debug utils messenger */
VkResult AurVK::CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto function = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
    if (function != nullptr) {
        return function(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    return VK_ERROR_EXTENSION_NOT_PRESENT;
}

/**
 * @param instance
 * @param debugMessenger
 * @param pAllocator
 */
/** Destroy debug utils messenger */
void AurVK::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto function = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
    if (function != nullptr) {
        function(instance, debugMessenger, pAllocator);
    }
}

/**
 * Todo -- Breakdown the Uniform binding process from here.
 * You may also need to update the Uniform Buffer as we are using the
 * same transformation matrix for all the vertices we're rendering
 */
/** Update uniform buffer */
void AurVK::updateUniformBuffer(uint32_t currentImage) {
    float aspectRatio = static_cast<float>(swapChainExtent.width) / static_cast<float>(swapChainExtent.height);

    //For Vertex Shader ubo
    UniformBufferObjectV uboV{};//uniform buffer object used to bind uniforms.//*UBO
    uboV.mvp = glm::ortho(-aspectRatio, aspectRatio, -1.0f, 1.0f, -1.0f, 1.0f); //transform the values for deformation

    //For Fragment shader ubo
    UniformBufferObjectF uboF{}; //*UBOF
    uboF.offset = 0.0f;

    UniformBufferObjectF uboF2{};
    uboF2.offset = 0.35f;

    void* data = nullptr;
    //The memory setup for vertex is being used.
    vkMapMemory(device, aDescriptor->getUniformBuffersMemoryV()[currentImage], 0, sizeof(uboV), 0, &data); //*UBO
    std::memcpy(data, &uboV, sizeof(uboV)); //Copy the values to the uniform buffer
    vkUnmapMemory(device, aDescriptor->getUniformBuffersMemoryV()[currentImage]);

    //remember to bind it for both/all descriptors that are setup for the objects
    vkMapMemory(device, aDescriptor2->getUniformBuffersMemoryV()[currentImage], 0, sizeof(uboV), 0, &data); //*UBO
    std::memcpy(data, &uboV, sizeof(uboV)); //Copy the values to the uniform buffer
    vkUnmapMemory(device, aDescriptor2->getUniformBuffersMemoryV()[currentImage]);

    //The memory setup for fragment is being used.
    vkMapMemory(device, aDescriptor->getUniformBuffersMemoryF()[currentImage], 0, sizeof(uboF), 0, &data); //*UBOF
    std::memcpy(data, &uboF, sizeof(uboF)); //Copy the values to the uniform buffer
    vkUnmapMemory(device, aDescriptor->getUniformBuffersMemoryF()[currentImage]);

    vkMapMemory(device, aDescriptor2->getUniformBuffersMemoryF()[currentImage], 0, sizeof(uboF2), 0, &data); //*UBOF
    std::memcpy(data, &uboF2, sizeof(uboF2)); //Copy the values to the uniform buffer
    vkUnmapMemory(device, aDescriptor2->getUniformBuffersMemoryF()[currentImage]);
}

/** Record command buffer */
void AurVK::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    AVKDebug::assertVKSuccess(vkBeginCommandBuffer(commandBuffer, &beginInfo), "begin command buffer");// start to Record commands

    VkClearValue clearColor{};
    clearColor.color = {{0.04f, 0.04f, 0.08f, 1.0f}};

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapChainExtent;
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;
    //start Render Pass
    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    //--------//*ORF
    /**
     * View Doc on different axis and mapping of orientation from OpenGL to Vulkan
     * https://www.saschawillems.de/blog/2019/03/29/flipping-the-vulkan-viewport/
     *
     * Methods to resolve :-
     * Either invert y in the fragment shader
     * change the winding order to counter clockwise.
     * Flip the viewport as per the above link doc.
     */
    VkViewport viewport{};//Todo -- define the viewport here as we'll like a viewport
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swapChainExtent.width);
    viewport.height = static_cast<float>(swapChainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    /**
     * Set scissor rectangles dynamically for a command buffer
     * The scissor rectangles taken from element i of pScissors replace the current state for the scissor index firstScissor + i, for i in [0, scissorCount).
     * Otherwise, this state is specified by the VkPipelineViewportStateCreateInfo::pScissors values used to create the currently active pipeline.
     */
    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapChainExtent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    VkDescriptorSet descriptorSet = aDescriptor->getDescriptorSets()[currentFrame];
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, aPipeline->getPipeline());//This needs to be unique
    //Bind Descriptor Set and
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, aPipeline->getPipelineLayout(), 0, 1, &descriptorSet, 0, nullptr);
    //Transforms need to be handled by the entity itself, structure this.
    aMesh1->render(commandBuffer, aPipeline->getPipelineLayout());

    /**
     * Temporary multi Mesh rendering and transformation
     * Todo - Need to structure this to have a cleaner Abstracted interface with Transforms , Textures and Shaders.
     */
    VkDescriptorSet descriptorSet2 = aDescriptor2->getDescriptorSets()[currentFrame];
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, aPipeline2->getPipeline());
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, aPipeline2->getPipelineLayout(), 0, 1, &descriptorSet2, 0, nullptr);
    aMesh2->render(commandBuffer, aPipeline2->getPipelineLayout());

    vkCmdEndRenderPass(commandBuffer);
    AVKDebug::assertVKSuccess(vkEndCommandBuffer(commandBuffer), "record command buffer");
}

/** Draw frame */
void AurVK::drawFrame() {
    vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex = 0;
    VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapChain();
        return;
    }
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        AVKDebug::assertVKSuccess(result, "acquire next image");
    }

    vkResetFences(device, 1, &inFlightFences[currentFrame]);
    updateUniformBuffer(currentFrame);// Updates the Uniforms in the shader

    VkCommandBuffer commandBuffer = aCommandBuffer->getCommandBuffers()[currentFrame];
    vkResetCommandBuffer(commandBuffer, 0);//resets the command buffer to be recreated
    //-------Record the commands before its is submitted to queue
    recordCommandBuffer(commandBuffer, imageIndex);//Todo -- command buffers are recorded

    VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;
    AVKDebug::assertVKSuccess(vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]), "submit draw command buffer");

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapChain;
    presentInfo.pImageIndices = &imageIndex;
    result = vkQueuePresentKHR(presentQueue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || orientationChange) {
        orientationChange = false;
        recreateSwapChain();
    } else if (result != VK_SUCCESS) {
        AVKDebug::assertVKSuccess(result, "present swapchain image");
    }

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;//buffering of frames will require to iterate over each to update drawings
}

/**
 * Destroy and cleanup gpu memory and context info.
 */
void AurVK::cleanup() {
    if (device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(device);
    }

    delete aMesh1;
    aMesh1 = nullptr;
    delete aMesh2;
    aMesh2 = nullptr;
    delete aTexture;
    aTexture = nullptr;
    delete aTexture2;
    aTexture2 = nullptr;
    delete aDescriptor;
    aDescriptor = nullptr;
    delete aDescriptor2;
    aDescriptor2 = nullptr;
    delete aPipeline;
    aPipeline = nullptr;
    delete aPipeline2;
    aPipeline2 = nullptr;
    delete aCommandBuffer;
    aCommandBuffer = nullptr;

    cleanupSwapChain();

    if (device != VK_NULL_HANDLE) {
        for (VkSemaphore semaphore : imageAvailableSemaphores) {
            vkDestroySemaphore(device, semaphore, nullptr);
        }
        imageAvailableSemaphores.clear();
        for (VkSemaphore semaphore : renderFinishedSemaphores) {
            vkDestroySemaphore(device, semaphore, nullptr);
        }
        renderFinishedSemaphores.clear();
        for (VkFence fence : inFlightFences) {
            vkDestroyFence(device, fence, nullptr);
        }
        inFlightFences.clear();

        if (renderPass != VK_NULL_HANDLE) {
            vkDestroyRenderPass(device, renderPass, nullptr);
            renderPass = VK_NULL_HANDLE;
        }
        vkDestroyDevice(device, nullptr);
        device = VK_NULL_HANDLE;
    }

    if (debugMessenger != VK_NULL_HANDLE) {
        DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
        debugMessenger = VK_NULL_HANDLE;
    }
    if (surface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(instance, surface, nullptr);
        surface = VK_NULL_HANDLE;
    }
    if (instance != VK_NULL_HANDLE) {
        vkDestroyInstance(instance, nullptr);
        instance = VK_NULL_HANDLE;
    }
    initialized = false;
}
