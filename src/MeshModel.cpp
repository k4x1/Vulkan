#include "MeshModel.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include <TINY/tiny_obj_loader.h>

void MeshModel::loadModel(const std::string& filepath) {
    modelMatrix = glm::mat4(1.0f); // Initialize model matrix
    vertices = ModelLoader::LoadModel(filepath);
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

void MeshModel::loadVBO(vk::Device& device, vk::PhysicalDevice& physicalDevice)
{

    vk::DeviceSize bufferSize = sizeof(VertexStandard) * vertices.size();

    vk::BufferCreateInfo bufferInfo = vk::BufferCreateInfo().setSize(bufferSize).setUsage(vk::BufferUsageFlagBits::eVertexBuffer);
    
    std::cout << &vertexBuffer << std::endl;

    if (device.createBuffer(&bufferInfo, nullptr, &vertexBuffer) != vk::Result::eSuccess) {
        throw std::runtime_error("Failed to create vertex buffer!");
    }
    std::cout << "post Buffer" << std::endl;

    vk::MemoryRequirements memRequirements;
    device.getBufferMemoryRequirements(vertexBuffer, &memRequirements);

    vk::MemoryAllocateInfo allocInfo = {};
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = ModelLoader::FindMemoryType(physicalDevice, memRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

    if (device.allocateMemory(&allocInfo, nullptr, &vertexBufferMemory) != vk::Result::eSuccess) {
        throw std::runtime_error("Failed to allocate vertex buffer memory!");
    }

    device.bindBufferMemory(vertexBuffer, vertexBufferMemory, 0);

    void* data;
    device.mapMemory(vertexBufferMemory, 0, VK_WHOLE_SIZE, {}, &data);
    memcpy(data, vertices.data(), (size_t)bufferSize);
    device.unmapMemory(vertexBufferMemory);

}

void MeshModel::cleanup(vk::Device& device, vk::PhysicalDevice& physicalDevice) {
    device.destroyBuffer(vertexBuffer);
    device.freeMemory(vertexBufferMemory);
}

glm::mat4* MeshModel::GetModelMat()
{
    return &modelMatrix;
}
