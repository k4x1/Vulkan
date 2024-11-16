#pragma once

#include <vulkan/vulkan.hpp>
#include "VertexStandard.h"
#include <vector>
#include <string>

class ModelLoader {
public:
    static std::vector<VertexStandard> LoadModel(const std::string& filepath, vk::Device device, vk::PhysicalDevice physicalDevice, vk::Buffer& vertexBuffer, vk::DeviceMemory& vertexBufferMemory);

private:
    static uint32_t FindMemoryType(vk::PhysicalDevice physicalDevice, uint32_t typeFilter, vk::MemoryPropertyFlags properties);
};
