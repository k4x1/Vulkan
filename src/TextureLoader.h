#pragma once

#include "common.h"

// WIP HERE: 
//
// Loading simple linear texture works perfectly.
// However, loading optimal tiling texture involves several hacky things. loadTexture is called three different times
// each does something slightly different. Details and more here: https://projects.cloudmillgames.com/cards/1348050913394165501
// I need to re-factor the optimal tiling texture based on my understanding so it's clean enough to be understood and
// works without redundant texture loads. An example of one of these issues is that loadTexture from original vkcube
// can actually only return size of texture without reading the texture

struct texture_object {
    vk::Sampler sampler;

    vk::Image image;
    vk::Buffer buffer;
    vk::ImageLayout imageLayout { vk::ImageLayout::eUndefined };

    vk::MemoryAllocateInfo mem_alloc;
    vk::DeviceMemory mem;
    vk::ImageView view;

    uint32_t tex_width { 0 };
    uint32_t tex_height { 0 };
};

class TextureLoader
{
    // About texture memory layouts on different GPU types:
	// * Desktop GPUs (PC, Playstation/XBOX) provide large linear video memory allowing textures to be loaded for direct linear row-major access
	// * Mobile GPUs (switch, mobile, tablets, etc) do not provide large linear video memory, textures have to be loaded into a platform-optimized tiled memory layout for efficient read/write

public:
	// Create texture in linear layout memory, direct loading memory (good for desktop GPUs)
    static void CreateTexture2D(const std::string& FileName, texture_object& TexObj);

	// Create texture using given flags and properties
	static void CreateTexture2DEx(const std::string& FileName, texture_object& TexObj, vk::ImageTiling Tiling, vk::ImageUsageFlags Usage, vk::MemoryPropertyFlags RequiredProps);

	// Creates an RGBA staging buffer
    static void CreateBuffer2D(const std::string& FileName, texture_object& TexObj);

    // Create texture in optimized layout memory, slow loading to optimize for efficient memory access (good for both desktop and mobile GPUs)
    static void CreateOptimalTexture2DFromBuffer(texture_object& source_buffer, texture_object& dest_texture);

	static void SetImageLayout(vk::Image image, vk::ImageAspectFlags aspectMask, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, vk::AccessFlags srcAccessMask, vk::PipelineStageFlags src_stages, vk::PipelineStageFlags dest_stages);

private:
    // Create a blank texture of arbitrary width and height using tiling, usage, and required properties
    static texture_object CreateTexture2DBlank(uint32_t TexWidth, uint32_t TexHeight, vk::ImageTiling Tiling, vk::ImageUsageFlags Usage, vk::MemoryPropertyFlags RequiredProps);
};
