cmake_minimum_required(VERSION 3.24)
project(Aura LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(vulkan_headers
    GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Headers.git
    GIT_TAG v1.4.367
    GIT_SHALLOW TRUE)
FetchContent_Declare(glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1
    GIT_SHALLOW TRUE)
FetchContent_Declare(stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG master
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(vulkan_headers glm stb)

# The Vulkan runtime ships without headers or an import library, so build the
# import library from the system loader's exports with the MSVC toolchain.
set(VULKAN_DLL "C:/Windows/System32/vulkan-1.dll")
if(NOT EXISTS "${VULKAN_DLL}")
    message(FATAL_ERROR "vulkan-1.dll was not found in C:/Windows/System32")
endif()
set(VK_IMPORT_DIR "${CMAKE_BINARY_DIR}/vulkan")
file(MAKE_DIRECTORY "${VK_IMPORT_DIR}")
set(VK_IMPLIB "${VK_IMPORT_DIR}/vulkan-1.lib")
if(NOT EXISTS "${VK_IMPLIB}")
    set(VSWHERE "C:/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe")
    execute_process(
        COMMAND "${VSWHERE}" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
        OUTPUT_VARIABLE VS_PATH
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    file(GLOB MSVC_BINS "${VS_PATH}/VC/Tools/MSVC/*/bin/Hostx64/x64")
    list(SORT MSVC_BINS COMPARE NATURAL ORDER DESCENDING)
    list(GET MSVC_BINS 0 MSVC_BIN)
    execute_process(
        COMMAND "${MSVC_BIN}/dumpbin.exe" /exports "${VULKAN_DLL}"
        OUTPUT_VARIABLE VULKAN_EXPORTS)
    string(REGEX MATCHALL "[ \t]+[0-9]+[ \t]+[0-9A-Fa-f]+[ \t]+[0-9A-Fa-f]+[ \t]+vk[A-Za-z0-9_]+" EXPORT_LINES "${VULKAN_EXPORTS}")
    set(VULKAN_DEF "LIBRARY vulkan-1.dll\nEXPORTS\n")
    foreach(line IN LISTS EXPORT_LINES)
        string(REGEX MATCH "vk[A-Za-z0-9_]+$" name "${line}")
        string(APPEND VULKAN_DEF "  ${name}\n")
    endforeach()
    file(WRITE "${VK_IMPORT_DIR}/vulkan-1.def" "${VULKAN_DEF}")
    execute_process(
        COMMAND "${MSVC_BIN}/lib.exe" /def:${VK_IMPORT_DIR}/vulkan-1.def /out:${VK_IMPLIB} /machine:x64
        RESULT_VARIABLE LIB_RESULT)
    if(NOT LIB_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to generate ${VK_IMPLIB}")
    endif()
endif()
add_library(VulkanLoader SHARED IMPORTED)
set_target_properties(VulkanLoader PROPERTIES
    IMPORTED_LOCATION "${VULKAN_DLL}"
    IMPORTED_IMPLIB "${VK_IMPLIB}")

add_executable(Aura WIN32
    main.cpp
    vk_utils.cpp
    vk_renderer.cpp
    vk_commandBuffers.cpp
    vk_descriptors.cpp
    vk_model.cpp
    vk_shaderpipeline.cpp
    vk_texture.cpp)
target_compile_definitions(Aura PRIVATE
    VK_USE_PLATFORM_WIN32_KHR
    NOMINMAX
    WIN32_LEAN_AND_MEAN
    _CRT_SECURE_NO_WARNINGS)
target_compile_options(Aura PRIVATE /utf-8 /W3)
target_include_directories(Aura PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${stb_SOURCE_DIR}")
target_link_libraries(Aura PRIVATE VulkanLoader Vulkan::Headers glm::glm user32 gdi32)

add_custom_command(TARGET Aura POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/shaders" "$<TARGET_FILE_DIR:Aura>/shaders"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/assets/texture1.png" "$<TARGET_FILE_DIR:Aura>/texture1.png"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/assets/texture.png" "$<TARGET_FILE_DIR:Aura>/texture.png")
