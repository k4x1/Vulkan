#pragma once

#ifdef _DEBUG
constexpr bool ENABLE_VULKAN_VALIDATION = true;
#else
constexpr bool ENABLE_VULKAN_VALIDATION = false;
#endif

constexpr char APP_SHORT_NAME [] = "mds-vkcubepp";
constexpr char PATH_TEXTURES [] = "resources/";
constexpr char PATH_SHADERS [] = "shaders/";

constexpr uint32_t WINDOW_WIDTH = 1280;
constexpr uint32_t WINDOW_HEIGHT = 720;

// Allow a maximum of two outstanding presentation operations.
constexpr uint32_t FRAME_LAG = 2;

constexpr char const* tex_files[] = {"vulkan.png"};

