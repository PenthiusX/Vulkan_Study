# Vulkan_Study

Windows port of the Android "Aura" Vulkan template. This project demonstrates modular Vulkan rendering with a focus on learning and experimentation.

## Features

- **Modular Vulkan wrappers**: instance, device, swapchain, render pass, framebuffers, pipeline, descriptors, command buffers, model, texture
- **Windows platform support**: WinMain entry point, HWND window management, Win32 surface creation
- **Asset management**: Custom `AssetManager` for loading textures and shaders
- **Shader pipeline**: Configurable graphics pipeline with vertex/fragment shaders
- **Model rendering**: Push constant support for model transformations
- **Texture handling**: Multiple texture support with sampler configuration
- **Debug utilities**: Vulkan validation layers and debug messenger

## Prerequisites

- **Windows 10/11**
- **Visual Studio 2019 or later** with C++ desktop development workload
- **CMake 3.20 or later**
- **Vulkan SDK** (1.3 or later)
  - Download from: https://vulkan.lunarg.com/sdk/home
  - Ensure `VULKAN_SDK` environment variable is set

## Dependencies

The following dependencies are automatically fetched by CMake:

- **glm** (OpenGL Mathematics) - Vector/matrix math library
- **stb** - Image loading (stb_image)
- **Vulkan Headers** - Vulkan API headers

## Building

### Using CMake (Recommended)

```bash
# Create build directory
mkdir build
cd build

# Configure with CMake
cmake .. -G "Visual Studio 17 2022" -A x64

# Build
cmake --build . --config Release
```

### Using Visual Studio

1. Open Visual Studio
2. Select "Open a local folder" and navigate to the project root
3. Visual Studio will automatically detect CMakeLists.txt
4. Select build configuration (x64-Release)
5. Build → Build All (Ctrl+Shift+B)

### Build Output

The executable will be generated at:
```
build/Release/Vulkan_Study.exe
```

## Running

```bash
cd build/Release
./Vulkan_Study.exe
```

The application opens a window with a textured quad rendered using Vulkan.

## Project Structure

```
Vulkan_Study/
├── main.cpp                    # Application entry point
├── CMakeLists.txt              # Build configuration
├── vk_engine.h                 # Main engine header
├── vk_renderer.h/cpp           # Core rendering logic
├── vk_model.h/cpp              # Model loading and rendering
├── vk_texture.h/cpp            # Texture management
├── vk_shaderpipeline.h/cpp     # Graphics pipeline setup
├── vk_descriptors.h/cpp        # Descriptor sets and layouts
├── vk_commandBuffers.h/cpp     # Command buffer management
├── vk_debughandler.h           # Debug utilities
├── vk_utils.h/cpp              # Helper functions
├── vk_vertex.h                 # Vertex definitions
├── assets/                     # Textures and resources
│   ├── texture.png
│   └── texture1.png
└── shaders/                    # GLSL shaders
    ├── shader.vert
    ├── shader.frag
    ├── shader.vert.spv         # Compiled SPIR-V
    └── shader.frag.spv
```

## Shaders

Shaders are written in GLSL and compiled to SPIR-V. Pre-compiled `.spv` files are included in the repository.

To recompile shaders (requires Vulkan SDK):

```bash
cd shaders
glslc shader.vert -o shader.vert.spv
glslc shader.frag -o shader.frag.spv
```

## Architecture Notes

This port preserves the original Android Aura architecture while adapting platform-specific components:

- `android_main` → `WinMain`
- `ANativeWindow` → `HWND`
- `AAssetManager` → Custom `AssetManager`
- `vkCreateAndroidSurfaceKHR` → Win32 surface creation

All Vulkan initialization, rendering, and resource management follows the original design patterns.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- Original Android "Aura" template by Aditya Mattoo
- Vulkan documentation and samples from Khronos Group
- [Learn Vulkan](https://vulkan-tutorial.com/) tutorial series

## Contributing

This is a study/reference project. For issues or questions, please open an issue on GitHub.
