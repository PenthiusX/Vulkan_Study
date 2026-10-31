#include "vk_utils.h"

#include <cstdarg>
#include <cstdio>
#include <fstream>
#include <stdexcept>

void vkLogInfo(const char* format, ...) {
    char buffer[1024];
    va_list args;
    va_start(args, format);
    std::vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    std::printf("%s\n", buffer);
    OutputDebugStringA(buffer);
    OutputDebugStringA("\n");
}

std::string assetPath(const char* path, AssetManager* assetManager) {
    if (assetManager == nullptr) {
        return path;
    }
    return assetManager->root + path;
}

std::vector<uint8_t> LoadBinaryFileToVector(const char* filePath, AssetManager* assetManager) {
    std::ifstream file(assetPath(filePath, assetManager), std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error(std::string("failed to open file: ") + filePath);
    }
    size_t fileSize = static_cast<size_t>(file.tellg());
    std::vector<uint8_t> buffer(fileSize);
    file.seekg(0);
    file.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(fileSize));
    return buffer;
}

std::vector<char> readFile(const char* filePath, AssetManager* assetManager) {
    std::vector<uint8_t> bytes = LoadBinaryFileToVector(filePath, assetManager);
    return std::vector<char>(bytes.begin(), bytes.end());
}
