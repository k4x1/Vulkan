// MeshModel.cpp
#include "MeshModel.h"
#include <stdexcept>
#include <iostream>
#include "ModelLoader.h"

MeshModel::MeshModel(const std::string& filename,
    vk::Device device,
    vk::PhysicalDevice physicalDevice,
    vk::CommandPool commandPool,
    vk::Queue graphicsQueue)
    : device(device),
    physicalDevice(physicalDevice),
    commandPool(commandPool),
    graphicsQueue(graphicsQueue) {
    try {
        vertices = ModelLoader::LoadModel(filename, device, physicalDevice, vertexBuffer, vertexBufferMemory);
    }
    catch (const std::exception& e) {
        throw std::runtime_error("Failed to load model: " + std::string(e.what()));
    }

}

MeshModel::~MeshModel() {
    cleanup();
}

void MeshModel::cleanup() {
    if (vertexBuffer) {
        device.destroyBuffer(vertexBuffer);
        vertexBuffer = VK_NULL_HANDLE;
    }
    if (vertexBufferMemory) {
        device.freeMemory(vertexBufferMemory);
        vertexBufferMemory = VK_NULL_HANDLE;
    }
}

void MeshModel::draw(const vk::CommandBuffer& commandBuffer) {
    vk::Buffer vertexBuffers[] = { vertexBuffer };
    vk::DeviceSize offsets[] = { 0 };
    commandBuffer.bindVertexBuffers(0, 1, vertexBuffers, offsets);
    commandBuffer.draw(static_cast<uint32_t>(vertices.size()), 1, 0, 0);
}
