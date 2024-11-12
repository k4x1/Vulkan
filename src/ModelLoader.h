#include <TINY/tiny_obj_loader.h>
#include <vulkan/vulkan.hpp>
#include "VertexStandard.h"
#include <vector>
#include <iostream>

class ModelLoader {
public:
    static std::vector<VertexStandard> LoadModel(const std::string& filepath, vk::Device device, vk::PhysicalDevice physicalDevice, vk::Buffer& vertexBuffer, vk::DeviceMemory& vertexBufferMemory) {
        // Load OBJ file
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filepath.c_str())) {
            throw std::runtime_error(warn + err);
        }

        // Extract vertex data
        std::vector<VertexStandard> vertices;
        for (const auto& shape : shapes) {
            for (const auto& index : shape.mesh.indices) {
                VertexStandard vertex = {};
                vertex.Position = {
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]
                };
                vertex.Normal = {
                    attrib.normals[3 * index.normal_index + 0],
                    attrib.normals[3 * index.normal_index + 1],
                    attrib.normals[3 * index.normal_index + 2]
                };
                vertex.TexCoord = {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    attrib.texcoords[2 * index.texcoord_index + 1]
                };
                vertices.push_back(vertex);
            }
        }

        // Create VBO
        vk::DeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

        vk::BufferCreateInfo bufferInfo = {};
        bufferInfo.size = bufferSize;
        bufferInfo.usage = vk::BufferUsageFlagBits::eVertexBuffer;
        bufferInfo.sharingMode = vk::SharingMode::eExclusive;

        if (device.createBuffer(&bufferInfo, nullptr, &vertexBuffer) != vk::Result::eSuccess) {
            throw std::runtime_error("Failed to create vertex buffer!");
        }

        vk::MemoryRequirements memRequirements;
        device.getBufferMemoryRequirements(vertexBuffer, &memRequirements);

        vk::MemoryAllocateInfo allocInfo = {};
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = FindMemoryType(physicalDevice, memRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

        if (device.allocateMemory(&allocInfo, nullptr, &vertexBufferMemory) != vk::Result::eSuccess) {
            throw std::runtime_error("Failed to allocate vertex buffer memory!");
        }

        device.bindBufferMemory(vertexBuffer, vertexBufferMemory, 0);

        void* data;
        device.mapMemory(vertexBufferMemory, 0, bufferSize, {}, &data);
        memcpy(data, vertices.data(), (size_t)bufferSize);
        device.unmapMemory(vertexBufferMemory);

        return vertices;
    }

private:
    static uint32_t FindMemoryType(vk::PhysicalDevice physicalDevice, uint32_t typeFilter, vk::MemoryPropertyFlags properties) {
        vk::PhysicalDeviceMemoryProperties memProperties;
        physicalDevice.getMemoryProperties(&memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }

        throw std::runtime_error("Failed to find suitable memory type!");
    }
};
