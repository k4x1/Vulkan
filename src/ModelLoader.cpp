#define TINYOBJLOADER_IMPLEMENTATION
#include <TINY/tiny_obj_loader.h>
#include "ModelLoader.h"
#include <stdexcept>
#include <iostream>

std::vector<VertexStandard> ModelLoader::LoadModel(const std::string& filepath, vk::Device device, vk::PhysicalDevice physicalDevice, vk::Buffer& vertexBuffer, vk::DeviceMemory& vertexBufferMemory) {
    std::cout << "Starting to load model from: " << filepath << std::endl;

    // Load OBJ file
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filepath.c_str())) {
        std::cerr << "Error loading OBJ file: " << warn << err << std::endl;
        throw std::runtime_error(warn + err);
    }
    std::cout << "OBJ file loaded successfully." << std::endl;

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
    std::cout << "Vertex data extracted. Total vertices: " << vertices.size() << std::endl;

    // Create VBO
    vk::DeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
    std::cout << "Buffer size calculated: " << bufferSize << " bytes" << std::endl;

    vk::BufferCreateInfo bufferInfo = {};
    bufferInfo.size = bufferSize;
    bufferInfo.usage = vk::BufferUsageFlagBits::eVertexBuffer;
    bufferInfo.sharingMode = vk::SharingMode::eExclusive;

    std::cout << "Creating vertex buffer with size: " << bufferSize << std::endl;

    vk::Result result = device.createBuffer(&bufferInfo, nullptr, &vertexBuffer);
    if (result != vk::Result::eSuccess) {
        std::cerr << "Failed to create vertex buffer! Error code: " << vk::to_string(result) << std::endl;
        throw std::runtime_error("Failed to create vertex buffer!");
    }
    std::cout << "Vertex buffer created successfully." << std::endl;


    std::cout << "Vertex buffer created successfully." << std::endl;

    vk::MemoryRequirements memRequirements;
    device.getBufferMemoryRequirements(vertexBuffer, &memRequirements);
    std::cout << "Memory requirements obtained." << std::endl;

    vk::MemoryAllocateInfo allocInfo = {};
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(physicalDevice, memRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

    if (device.allocateMemory(&allocInfo, nullptr, &vertexBufferMemory) != vk::Result::eSuccess) {
        std::cerr << "Failed to allocate vertex buffer memory!" << std::endl;
        throw std::runtime_error("Failed to allocate vertex buffer memory!");
    }
    std::cout << "Vertex buffer memory allocated successfully." << std::endl;

    device.bindBufferMemory(vertexBuffer, vertexBufferMemory, 0);
    std::cout << "Vertex buffer memory bound successfully." << std::endl;

    void* data;
    device.mapMemory(vertexBufferMemory, 0, bufferSize, {}, &data);
    memcpy(data, vertices.data(), (size_t)bufferSize);
    device.unmapMemory(vertexBufferMemory);
    std::cout << "Vertex data copied to buffer memory." << std::endl;

    return vertices;
}

uint32_t ModelLoader::FindMemoryType(vk::PhysicalDevice physicalDevice, uint32_t typeFilter, vk::MemoryPropertyFlags properties) {
    vk::PhysicalDeviceMemoryProperties memProperties;
    physicalDevice.getMemoryProperties(&memProperties);
    std::cout << "Finding suitable memory type." << std::endl;

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            std::cout << "Suitable memory type found: " << i << std::endl;
            return i;
        }
    }

    std::cerr << "Failed to find suitable memory type!" << std::endl;
    throw std::runtime_error("Failed to find suitable memory type!");
}
 