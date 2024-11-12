#pragma once

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include "VertexStandard.h"
#include <string>
#include <vector>

class MeshModel {
public:
    MeshModel(const std::string& filename, vk::Device device, vk::PhysicalDevice gpu, vk::CommandPool commandPool, vk::Queue graphicsQueue);
    ~MeshModel();

    void draw(vk::CommandBuffer commandBuffer);

private:
    void loadModel(const std::string& filename);
    void createVertexBuffer();
    void createIndexBuffer();
    void createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Buffer& buffer, vk::DeviceMemory& bufferMemory);

    uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);
    void copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size);

    vk::Device device;
    vk::PhysicalDevice physicalDevice;
    vk::CommandPool commandPool;
    vk::Queue graphicsQueue;

    std::vector<VertexStandard> vertices;
    std::vector<uint32_t> indices;

    vk::Buffer vertexBuffer;
    vk::DeviceMemory vertexBufferMemory;
    vk::Buffer indexBuffer;
    vk::DeviceMemory indexBufferMemory;
};
