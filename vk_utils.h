#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <cstdint>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

#define LOGI(...) vkLogInfo(__VA_ARGS__)

//Constants hard coded values , should be procedural.
constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 5;//Should be replaced by swapchain images size but failing in some cases. Todo -- check

struct AssetManager {
    std::string root;
};

void vkLogInfo(const char* format, ...);
std::string assetPath(const char* path, AssetManager* assetManager);
std::vector<uint8_t> LoadBinaryFileToVector(const char* filePath, AssetManager* assetManager);
std::vector<char> readFile(const char* filePath, AssetManager* assetManager);

struct QueueFamilyIndices {
    int graphicsFamily = -1;
    int presentFamily = -1;

    bool isComplete() const {
        return graphicsFamily >= 0 && presentFamily >= 0;
    }
};
