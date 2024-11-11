#include "MeshModel.h"
#include "Utils.h"
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

void MeshModel::UpdateUniformBuffer(vk::DeviceMemory uniformMemory, const Utils::UniformBufferObject& ubo) {
    void* data;
    m_device.mapMemory(uniformMemory, 0, sizeof(Utils::UniformBufferObject), vk::MemoryMapFlags(), &data);
    memcpy(data, &ubo, sizeof(Utils::UniformBufferObject));
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

    std::vector<Utils::Vertex> vertices;
    std::vector<uint32_t> indices;

    CreateVertexBuffer(vertices);
    CreateIndexBuffer(indices);
}

void MeshModel::CreateVertexBuffer(const std::vector<Utils::Vertex>& vertices) {
    m_vertexCount = static_cast<uint32_t>(vertices.size());
    vk::DeviceSize bufferSize = sizeof(vertices[0]) * m_vertexCount;

    vk::Buffer stagingBuffer;
    vk::DeviceMemory stagingBufferMemory;
    Utils::createBuffer(m_device, m_physicalDevice, bufferSize, vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuffer, stagingBufferMemory);

    void* data; 
    m_device.mapMemory(stagingBufferMemory, 0, bufferSize, vk::MemoryMapFlags(), &data);
    memcpy(data, vertices.data(), bufferSize);
    m_device.unmapMemory(stagingBufferMemory);

    Utils::createBuffer(m_device, m_physicalDevice, bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
        vk::MemoryPropertyFlagBits::eDeviceLocal, m_vertexBuffer, m_vertexBufferMemory);

    Utils::copyBuffer(m_device, m_graphicsQueue, stagingBuffer, m_vertexBuffer, bufferSize);

    m_device.destroyBuffer(stagingBuffer);
    m_device.freeMemory(stagingBufferMemory);
}

void MeshModel::CreateIndexBuffer(const std::vector<uint32_t>& indices) {
    m_indexCount = static_cast<uint32_t>(indices.size());
    vk::DeviceSize bufferSize = sizeof(indices[0]) * m_indexCount;

    vk::Buffer stagingBuffer;
    vk::DeviceMemory stagingBufferMemory;
    Utils::createBuffer(m_device, m_physicalDevice, bufferSize, vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuffer, stagingBufferMemory);

    void* data;
    m_device.mapMemory(stagingBufferMemory, 0, bufferSize, vk::MemoryMapFlags(), &data);
    memcpy(data, indices.data(), bufferSize);
    m_device.unmapMemory(stagingBufferMemory);

    Utils::createBuffer(m_device, m_physicalDevice, bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
        vk::MemoryPropertyFlagBits::eDeviceLocal, m_indexBuffer, m_indexBufferMemory);

    Utils::copyBuffer(m_device, m_graphicsQueue, stagingBuffer, m_indexBuffer, bufferSize);

    m_device.destroyBuffer(stagingBuffer);
    m_device.freeMemory(stagingBufferMemory);
}

glm::mat4 MeshModel::CalculateModelMatrix() {
    glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), m_position);
    glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(m_rotation.y), glm::vec3(0, 1, 0))
        * glm::rotate(glm::mat4(1.0f), glm::radians(m_rotation.x), glm::vec3(1, 0, 0))
        * glm::rotate(glm::mat4(1.0f), glm::radians(m_rotation.z), glm::vec3(0, 0, 1));
    glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), m_scale);

    return translationMatrix * rotationMatrix * scaleMatrix;
}

