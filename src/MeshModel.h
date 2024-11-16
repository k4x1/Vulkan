// MeshModel.h
#pragma once

#include <vulkan/vulkan.hpp>
#include "VertexStandard.h"
#include <vector>

class ModelLoader;

class MeshModel {
public:
    MeshModel(const std::string& filepath,
        vk::Device device,
        vk::PhysicalDevice physicalDevice,
        vk::CommandPool commandPool,
        vk::Queue graphicsQueue);
    ~MeshModel();

    void draw(const vk::CommandBuffer& commandBuffer);
    void cleanup();

private:
    // Member variables
    vk::Device device;
    vk::PhysicalDevice physicalDevice;
    vk::CommandPool commandPool;
    vk::Queue graphicsQueue;

    vk::Buffer vertexBuffer;
    vk::DeviceMemory vertexBufferMemory;

    std::vector<VertexStandard> vertices;
};
