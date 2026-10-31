#pragma once

#include "vk_renderer.h"

struct VulkanEngine {
    HWND window{};
    AurVK* app_backend{};
    bool canRender{};
};
