#pragma once

#include "MeshModel.h"
#include <iostream>
#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "VertexStandard.h"
#include <unordered_map>
#define TINYOBJLOADER_IMPLEMENTATION
#include <TINY/tiny_obj_loader.h>


MeshModel::MeshModel(vk::Device device, vk::PhysicalDevice physicalDevice, vk::CommandPool commandPool, vk::Queue graphicsQueue,
    glm::vec3 position, glm::vec3 rotation, glm::vec3 scale, const std::string& modelFilePath)
    : m_device(device), m_physicalDevice(physicalDevice), m_commandPool(commandPool), m_graphicsQueue(graphicsQueue),
    m_position(position), m_rotation(rotation), m_scale(scale) {
    LoadModel(modelFilePath);
    m_modelMatrix = CalculateModelMatrix();
}

MeshModel::~MeshModel() {
    m_device.destroyBuffer(m_vertexBuffer);
    m_device.freeMemory(m_vertexBufferMemory);
    m_device.destroyBuffer(m_indexBuffer);
    m_device.freeMemory(m_indexBufferMemory);
}

void MeshModel::Update(float deltaTime) {
    // Update any animation or dynamic properties of the mesh model
}

void MeshModel::Render(vk::CommandBuffer commandBuffer, vk::Pipeline pipeline, vk::PipelineLayout pipelineLayout, vk::DescriptorSet descriptorSet)
{
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

    vk::Buffer vertexBuffers[] = { m_vertexBuffer };
    vk::DeviceSize offsets[] = { 0 };
    commandBuffer.bindVertexBuffers(0, 1, vertexBuffers, offsets);
    commandBuffer.bindIndexBuffer(m_indexBuffer, 0, vk::IndexType::eUint32);

    commandBuffer.drawIndexed(m_indexCount, 1, 0, 0, 0);
}

void MeshModel::SetPosition(const glm::vec3& newPos) {
    m_position = newPos;
    m_modelMatrix = CalculateModelMatrix();
}

void MeshModel::SetRotation(const glm::vec3& newRot) {
    m_rotation = newRot;
    m_modelMatrix = CalculateModelMatrix();
}

void MeshModel::SetScale(const glm::vec3& newScale) {
    m_scale = newScale;
    m_modelMatrix = CalculateModelMatrix();
}

void MeshModel::SetTexture(vk::ImageView textureImageView, vk::Sampler textureSampler) {
    m_textureImageView = textureImageView;
    m_textureSampler = textureSampler;
}

void MeshModel::UpdateUniformBuffer(vk::DeviceMemory uniformMemory, const glm::mat4& modelMatrix) {
    void* data;
    m_device.mapMemory(uniformMemory, 0, sizeof(glm::mat4), vk::MemoryMapFlags(), &data);
    memcpy(data, glm::value_ptr(modelMatrix), sizeof(glm::mat4));
    m_device.unmapMemory(uniformMemory);
}
void MeshModel::LoadModel(const std::string& modelFilePath) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, modelFilePath.c_str())) {
        throw std::runtime_error(warn + err);
    }

    std::vector<VertexStandard> vertices;
    std::vector<uint32_t> indices;

    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            VertexStandard vertex{};

            vertex.Position = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };

            if (index.normal_index >= 0) {
                vertex.Normal = {
                    attrib.normals[3 * index.normal_index + 0],
                    attrib.normals[3 * index.normal_index + 1],
                    attrib.normals[3 * index.normal_index + 2]
                };
            }

            if (index.texcoord_index >= 0) {
                vertex.TexCoord = {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    attrib.texcoords[2 * index.texcoord_index + 1]
                };
            }

            // Linear search to find if the vertex already exists
            auto it = std::find(vertices.begin(), vertices.end(), vertex);
            if (it != vertices.end()) {
                indices.push_back(static_cast<uint32_t>(std::distance(vertices.begin(), it)));
            }
            else {
                vertices.push_back(vertex);
                indices.push_back(static_cast<uint32_t>(vertices.size() - 1));
            }
        }
    }

    CreateVertexBuffer(vertices);
    CreateIndexBuffer(indices);
}

void MeshModel::CreateVertexBuffer(const std::vector<VertexStandard>& vertices) {
    m_vertexCount = static_cast<uint32_t>(vertices.size());
    vk::DeviceSize bufferSize = sizeof(vertices[0]) * m_vertexCount;

    vk::Buffer stagingBuffer;
    vk::DeviceMemory stagingBufferMemory;
    CreateBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuffer, stagingBufferMemory);

    void* data;
    m_device.mapMemory(stagingBufferMemory, 0, bufferSize, vk::MemoryMapFlags(), &data);
    memcpy(data, vertices.data(), bufferSize);
    m_device.unmapMemory(stagingBufferMemory);

    CreateBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal, m_vertexBuffer, m_vertexBufferMemory);

    CopyBuffer(stagingBuffer, m_vertexBuffer, bufferSize);

    m_device.destroyBuffer(stagingBuffer);
    m_device.freeMemory(stagingBufferMemory);
}

void MeshModel::CreateIndexBuffer(const std::vector<uint32_t>& indices) {
    m_indexCount = static_cast<uint32_t>(indices.size());
    vk::DeviceSize bufferSize = sizeof(indices[0]) * m_indexCount;

    vk::Buffer stagingBuffer;
    vk::DeviceMemory stagingBufferMemory;
    CreateBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuffer, stagingBufferMemory);

    void* data;
    m_device.mapMemory(stagingBufferMemory, 0, bufferSize, vk::MemoryMapFlags(), &data);
    memcpy(data, indices.data(), bufferSize);
    m_device.unmapMemory(stagingBufferMemory);

    CreateBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal, m_indexBuffer, m_indexBufferMemory);

    CopyBuffer(stagingBuffer, m_indexBuffer, bufferSize);

    m_device.destroyBuffer(stagingBuffer);
    m_device.freeMemory(stagingBufferMemory);
}
void MeshModel::CreateBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Buffer& buffer, vk::DeviceMemory& bufferMemory) {
    vk::BufferCreateInfo bufferInfo{};
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = vk::SharingMode::eExclusive;

    if (m_device.createBuffer(&bufferInfo, nullptr, &buffer) != vk::Result::eSuccess) {
        throw std::runtime_error("failed to create buffer!");
    }

    vk::MemoryRequirements memRequirements;
    m_device.getBufferMemoryRequirements(buffer, &memRequirements);

    vk::MemoryAllocateInfo allocInfo{};
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

    if (m_device.allocateMemory(&allocInfo, nullptr, &bufferMemory) != vk::Result::eSuccess) {
        throw std::runtime_error("failed to allocate buffer memory!");
    }

    m_device.bindBufferMemory(buffer, bufferMemory, 0);
}

void MeshModel::CopyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size) {
    vk::CommandBufferAllocateInfo allocInfo{};
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandPool = m_commandPool;
    allocInfo.commandBufferCount = 1;

    vk::CommandBuffer commandBuffer;
    m_device.allocateCommandBuffers(&allocInfo, &commandBuffer);

    vk::CommandBufferBeginInfo beginInfo{};
    beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

    commandBuffer.begin(&beginInfo);

    vk::BufferCopy copyRegion{};
    copyRegion.size = size;
    commandBuffer.copyBuffer(srcBuffer, dstBuffer, 1, &copyRegion);

    commandBuffer.end();

    vk::SubmitInfo submitInfo{};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    m_graphicsQueue.submit(1, &submitInfo, nullptr);
    m_graphicsQueue.waitIdle();

    m_device.freeCommandBuffers(m_commandPool, 1, &commandBuffer);
}

glm::mat4 MeshModel::CalculateModelMatrix() {
    glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), m_position);
    glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(m_rotation.y), glm::vec3(0, 1, 0))
        * glm::rotate(glm::mat4(1.0f), glm::radians(m_rotation.x), glm::vec3(1, 0, 0))
        * glm::rotate(glm::mat4(1.0f), glm::radians(m_rotation.z), glm::vec3(0, 0, 1));
    glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), m_scale);

    return translationMatrix * rotationMatrix * scaleMatrix;
}
uint32_t MeshModel::FindMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) {
    vk::PhysicalDeviceMemoryProperties memProperties;
    m_physicalDevice.getMemoryProperties(&memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}