#include "MeshModel.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include <TINY/tiny_obj_loader.h>
MeshModel::MeshModel(vk::Device dev, vk::PhysicalDevice phyDev)
{
    device = dev;
    physicalDevice = phyDev;
}

void MeshModel::loadModel(const std::string& filepath) {
    vertices = ModelLoader::LoadModel(filepath, device, physicalDevice, vertexBuffer, vertexBufferMemory);
    modelMatrix = glm::mat4(1.0f); // Initialize model matrix
}

void MeshModel::render(const vk::CommandBuffer& commandBuffer) {
    vk::Buffer vertexBuffers[] = { vertexBuffer };
    vk::DeviceSize offsets[] = { 0 };
    commandBuffer.bindVertexBuffers(0, vertexBuffers, offsets);
    commandBuffer.draw(static_cast<uint32_t>(vertices.size()), 1, 0, 0);
}

void MeshModel::updateUniformData(glm::mat4 modelMatrix, glm::mat4 viewProjMatrix, void* uniformMemoryPtr) {
    UBO_Textured uniformData;
    uniformData.model = modelMatrix;
    uniformData.viewproj = viewProjMatrix;
    memcpy(uniformMemoryPtr, &uniformData, sizeof(UBO_Textured));
}

void MeshModel::cleanup() {
    device.destroyBuffer(vertexBuffer);
    device.freeMemory(vertexBufferMemory);
}
