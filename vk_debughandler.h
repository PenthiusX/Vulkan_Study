#pragma once

#include <cstdio>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan.h>

#include "vk_utils.h"

class AVKDebug {
public:
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT,
        const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
        void*) {//*VA
        if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
            LOGI("validation: %s", callbackData->pMessage);
        }
        return VK_FALSE;
    }

    static void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {//*VA
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                     VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                     VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
    }

    static void assertVKSuccess(VkResult result, const char* what) {
        if (result == VK_SUCCESS) {
            return;
        }
        char message[512];
        std::snprintf(message, sizeof(message), "%s failed (VkResult %d)", what, static_cast<int>(result));
        LOGI("%s", message);
        MessageBoxA(nullptr, message, "Aura", MB_ICONERROR | MB_OK);
        throw std::runtime_error(message);
    }

    static std::vector<const char*> getRequiredInstanceExtensions() {
        return {
            "VK_KHR_surface",
            "VK_KHR_win32_surface",
            VK_EXT_DEBUG_UTILS_EXTENSION_NAME
        };
    }
};
