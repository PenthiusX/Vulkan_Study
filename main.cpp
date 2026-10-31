/**
 * An easy to follow Vulkan template for Android.
 * Features -
 * Vulkan Renderer with Swapchain, device, pipeline and command buffer setup
 * Model Loading from external data.(no model loading library attached)
 * Shader Uniforms , Push constants and Attribute loading
 * Texturing of Primitives.
 *
 * (Alpha)Multi Transformable model support.
 * (Not covering individual shaders and textures for entities will require Restructuring).
 */

/**
 * Author : Aditya Mattoo // Created By a.mattoo on 20-07-2024.
 *
 * This is a Reference Template for Vulkan on Android.
 * This is based out of a Mix of VK tutorial one page template setup and Androids VK tutorial.
 *
 * Separator Font : ANSI Shadow
 */

//Glossary/Search Markers :
/**
 * *VA -- Validation.
 * *SL -- Shader layout specific changes. Bindings in shader.(Texture, attributes, etc)
 * *UBO -- update uniform/setting variables in shader.
 * *TEX -- Texturing. Todo -- Texture Requires
 * *MTEX -- Multiple Textures same shader. Todo -- Texture Requires
 * *EML -- External Mesh Loading
 * *MML -- Multiple Mesh Loading. Todo -- Descriptor Set Layout(descriptorSetLayout) , Graphics Pipeline(graphicsPipeline)
 * *ORF -- Ogl(convention) to Vk(convention)
 * *PUC -- Push Constants
 * // For Passing uniforms to fragment shaders , this holds simultaneous passes to fragment and vertex shader.
 * *UBOF -- Setting up to push uniform buffers to fragment shader (in this test it pushes to Both Vertex and fragment shader).
 * *B64 -- Base 64 implementation steps.
 */

//Vulkan main objects and their use
/**
 * VkInstance : The Vulkan context, used to access drivers.
 * VkPhysicalDevice : A GPU. Used to query physical GPU details, like features, capabilities, memory size, etc.
 * VkDevice : The "logical" GPU context that you actually execute things on.
 * VkBuffer : A chunk of GPU visible memory.
 * VkImage : A texture you can write to and read from.
 * VkPipeline : Holds the state of the gpu needed to draw. For example: shaders, rasterization options, depth settings.
 * VkRenderPass : Holds information about the images you are rendering into. All drawing commands have to be done inside a render pass.
 * VkFrameBuffer : Holds the target images for a render pass. Only used in legacy vk guide.
 * VkCommandBuffer : Encodes GPU commands. All execution that is performed on the GPU itself (not in the driver) has to be recorded into a command buffer.
 * VkCommandPool : Command pools are opaque objects that command buffer memory is allocated from, and which allow the implementation to amortize the cost of resource creation across multiple command buffers.
 * VkQueue : Execution "port" for commands. GPUs will have a set of queues with different properties. Some allow only graphics commands, others allow only compute commands, and others allow transfer of memory. A graphics queue is mandatory.
 * VkDescriptorSet : Holds the binding information that connects shader inputs to data such as VkBuffer resources and VkImageViews. They are bound during a command buffer.
 * VkSwapchainKHR : Holds the images for the screen. It allows you to render things into a visible window. The KHR suffix indicates that the functionality is provided by an extension.
 * VkSemaphore : Synchronizes GPU to GPU execution of commands. Used for syncing multiple command buffer submissions when using multiple queues.
 * VkFence : Synchronizes GPU to CPU execution of commands. Used to know if a command buffer has finished being executed on the GPU.
 */

//
#include <string>

#include "vk_engine.h"

namespace {
    
    std::string executableDirectory() {
        char path[MAX_PATH]{};
        GetModuleFileNameA(nullptr, path, MAX_PATH);
        std::string fullPath(path);
        size_t separator = fullPath.find_last_of("\\/");
        return separator == std::string::npos ? std::string() : fullPath.substr(0, separator + 1);
    }

    LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
        //Key inputs
        switch (message) {
        case WM_KEYDOWN:
            //Key input updates
            if (wParam == VK_ESCAPE) {
                DestroyWindow(hwnd);
                return 0;
            }
            break;
        case WM_CLOSE:
            // The window is being closed, clean it up.
            DestroyWindow(hwnd);
            return 0;
        case WM_DESTROY:
            //After context is closed.
            PostQuitMessage(0); //On Shutdown
            return 0;
        }
        return DefWindowProc(hwnd, message, wParam, lParam);
    }
}

/**
 * Entry point required by the Android Glue library.
 * This can also be achieved more verbosely by manually declaring JNI functions
 * and calling them from the Android application layer.
 */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    WNDCLASSA windowClass{};
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = hInstance;
    windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    windowClass.lpszClassName = "AuraWindowClass";
    RegisterClassA(&windowClass);

    RECT windowRect{0, 0, 1280, 720};
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);
    HWND window = CreateWindowExA(0, windowClass.lpszClassName, "Aura",
                                  WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                                  windowRect.right - windowRect.left,
                                  windowRect.bottom - windowRect.top,
                                  nullptr, nullptr, hInstance, nullptr);
    if (window == nullptr) {
        return 0;
    }

    //
    VulkanEngine engine{};
    engine.window = window;

    AurVK vulkanBackend{}; //Init the references for the Vulkan Renderer
    engine.app_backend = &vulkanBackend; //pass the reference used in HandleCmd2();
    //

    AssetManager assets{executableDirectory()};
    try {
        engine.app_backend->reset(window, &assets); //init vulkan here
        engine.canRender = engine.app_backend->initialized;
    } catch (const std::exception& exception) {
        MessageBoxA(nullptr, exception.what(), "Aura", MB_ICONERROR | MB_OK);
        return 1;
    }

    ShowWindow(window, nCmdShow);

    MSG message{};
    bool running = true;
    while (running) {
        while (PeekMessageA(&message, nullptr, 0, 0, PM_REMOVE)) {
            if (message.message == WM_QUIT) {
                running = false;
                break;
            }
            TranslateMessage(&message);
            DispatchMessageA(&message);
        }
        if (!running) {
            break;
        }
        if (engine.canRender) {
            //Run VK renderer function in Loop
            engine.app_backend->render();
        }
    }
    return 0;
}
