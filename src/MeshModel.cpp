#include "MeshModel.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include <TINY/tiny_obj_loader.h>
#include <stdexcept>
#include <iostream>

MeshModel::MeshModel(const std::string& filename, vk::Device device, vk::PhysicalDevice gpu, vk::CommandPool commandPool, vk::Queue graphicsQueue)
    : device(device), physicalDevice(gpu), commandPool(commandPool), graphicsQueue(graphicsQueue)
{
    loadModel(filename);
    createVertexBuffer();
    createIndexBuffer();
}

MeshModel::~MeshModel()
{
    device.destroyBuffer(indexBuffer);
    device.freeMemory(indexBufferMemory);

    device.destroyBuffer(vertexBuffer);
    device.freeMemory(vertexBufferMemory);
}

void MeshModel::loadModel(const std::string& filename)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    // Load OBJ file
    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename.c_str());
    if (!warn.empty()) {
        std::cout << "TinyObjLoader Warning: " << warn << std::endl;
    }
    if (!err.empty()) {
        std::cerr << "TinyObjLoader Error: " << err << std::endl;
    }
    if (!ret) {
        throw std::runtime_error("Failed to load model!");
    }

    // Loop over shapes and faces
    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            VertexStandard vertex = {};

            vertex.Position = {
                attrib.vertices[3 * index.vertex_index + 0], // X
                attrib.vertices[3 * index.vertex_index + 1], // Y
                attrib.vertices[3 * index.vertex_index + 2]  // Z
            };

            if (!attrib.normals.empty()) {
                vertex.Normal = {
                    attrib.normals[3 * index.normal_index + 0], // NX
                    attrib.normals[3 * index.normal_index + 1], // NY
                    attrib.normals[3 * index.normal_index + 2]  // NZ
                };
            }
            else {
                vertex.Normal = glm::vec3(0.0f);
            }

            if (!attrib.texcoords.empty()) {
                vertex.TexCoord = {
                    attrib.texcoords[2 * index.texcoord_index + 0],        // U
                    1.0f - attrib.texcoords[2 * index.texcoord_index + 1] // V (flip)
                };
            }
            else {
                vertex.TexCoord = glm::vec2(0.0f);
            }

            // Add the vertex to the list
            vertices.push_back(vertex);
            // The index is simply the current index of the vertex in the vector
            indices.push_back(static_cast<uint32_t>(vertices.size() - 1));
        }
    }
}

void MeshModel::createVertexBuffer()
{
    vk::DeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

    // Create staging buffer
    vk::Buffer stagingBuffer;
    vk::DeviceMemory stagingBufferMemory;
    createBuffer(bufferSize,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        stagingBuffer,
        stagingBufferMemory);

    // Copy vertex data to staging buffer
    void* data = device.mapMemory(stagingBufferMemory, 0, bufferSize);
    memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
    device.unmapMemory(stagingBufferMemory);

    // Create vertex buffer
    createBuffer(bufferSize,
        vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        vertexBuffer,
        vertexBufferMemory);

    // Copy data from staging buffer to vertex buffer
    copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

    // Clean up staging buffer
    device.destroyBuffer(stagingBuffer);
    device.freeMemory(stagingBufferMemory);
}

void MeshModel::createIndexBuffer()
{
    vk::DeviceSize bufferSize = sizeof(indices[0]) * indices.size();

    // Create staging buffer
    vk::Buffer stagingBuffer;
    vk::DeviceMemory stagingBufferMemory;
    createBuffer(bufferSize,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        stagingBuffer,
        stagingBufferMemory);

    // Copy index data to staging buffer
    void* data = device.mapMemory(stagingBufferMemory, 0, bufferSize);
    memcpy(data, indices.data(), static_cast<size_t>(bufferSize));
    device.unmapMemory(stagingBufferMemory);

    // Create index buffer
    createBuffer(bufferSize,
        vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        indexBuffer,
        indexBufferMemory);

    // Copy data from staging buffer to index buffer
    copyBuffer(stagingBuffer, indexBuffer, bufferSize);

    // Clean up staging buffer
    device.destroyBuffer(stagingBuffer);
    device.freeMemory(stagingBufferMemory);
}
void MeshModel::createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties,
    vk::Buffer& buffer, vk::DeviceMemory& bufferMemory)
{
    vk::BufferCreateInfo bufferInfo{};
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = vk::SharingMode::eExclusive;

    if (device.createBuffer(&bufferInfo, nullptr, &buffer) != vk::Result::eSuccess) {
        throw std::runtime_error("Failed to create buffer!");
    }

    vk::MemoryRequirements memRequirements = device.getBufferMemoryRequirements(buffer);

    vk::MemoryAllocateInfo allocInfo{};
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    if (device.allocateMemory(&allocInfo, nullptr, &bufferMemory) != vk::Result::eSuccess) {
        throw std::runtime_error("Failed to allocate buffer memory!");
    }

    device.bindBufferMemory(buffer, bufferMemory, 0);
}


uint32_t MeshModel::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties)
{
    vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice.getMemoryProperties();

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("Failed to find suitable memory type!");
}

void MeshModel::copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size)
{
    vk::CommandBufferAllocateInfo allocInfo{};
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    vk::CommandBuffer commandBuffer;
    device.allocateCommandBuffers(&allocInfo, &commandBuffer);

    vk::CommandBufferBeginInfo beginInfo{};
    beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

    commandBuffer.begin(beginInfo);

    vk::BufferCopy copyRegion{};
    copyRegion.size = size;
    commandBuffer.copyBuffer(srcBuffer, dstBuffer, copyRegion);

    commandBuffer.end();

    vk::SubmitInfo submitInfo{};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    graphicsQueue.submit(submitInfo, nullptr);
    graphicsQueue.waitIdle();

    device.freeCommandBuffers(commandPool, commandBuffer);
}


void MeshModel::draw(vk::CommandBuffer commandBuffer)
{
    vk::Buffer vertexBuffers[] = { vertexBuffer };
    vk::DeviceSize offsets[] = { 0 };
    commandBuffer.bindVertexBuffers(0, 1, vertexBuffers, offsets);

    commandBuffer.bindIndexBuffer(indexBuffer, 0, vk::IndexType::eUint32);

    commandBuffer.drawIndexed(static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
}