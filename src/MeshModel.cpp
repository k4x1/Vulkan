#include "MeshModel.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include <TINY/tiny_obj_loader.h>

void MeshModel::loadModel(const std::string& filepath) {
    modelMatrix = glm::mat4(1.0f); // Initialize model matrix
    vertices.clear();
    indices.clear();

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filepath.c_str())) {
        throw std::runtime_error(warn + err);
    }

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
            indices.push_back(static_cast<uint32_t>(vertices.size() - 1));
        }
    }
}

void MeshModel::render(const vk::CommandBuffer& commandBuffer) {
    vk::Buffer vertexBuffers[] = { vertexBuffer };
    vk::DeviceSize offsets[] = { 0 };
    commandBuffer.bindVertexBuffers(0, vertexBuffers, offsets);
    commandBuffer.bindIndexBuffer(indexBuffer, 0, vk::IndexType::eUint32);
    commandBuffer.drawIndexed(static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
}

void MeshModel::updateUniformData(glm::mat4 viewProjMatrix, void* uniformMemoryPtr) {
    UBO_Textured uniformData;
    uniformData.model = modelMatrix;
    uniformData.viewproj = viewProjMatrix;
    memcpy(uniformMemoryPtr, &uniformData, sizeof(UBO_Textured));
}
void MeshModel::loadVBO(vk::Device& device, vk::PhysicalDevice& physicalDevice) {
    this->device = &device;
    this->physicalDevice = &physicalDevice;

    // Vertex buffer
    vk::DeviceSize vertexBufferSize = sizeof(VertexStandard) * vertices.size();
    vk::BufferCreateInfo bufferInfo = vk::BufferCreateInfo()
        .setSize(vertexBufferSize)
        .setUsage(vk::BufferUsageFlagBits::eVertexBuffer);

    if (device.createBuffer(&bufferInfo, nullptr, &vertexBuffer) != vk::Result::eSuccess) {
        throw std::runtime_error("Failed to create vertex buffer!");
    }

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
    memcpy(data, vertices.data(), (size_t)vertexBufferSize);
    device.unmapMemory(vertexBufferMemory);

    // Index buffer
    vk::DeviceSize indexBufferSize = sizeof(uint32_t) * indices.size();
    bufferInfo.setSize(indexBufferSize).setUsage(vk::BufferUsageFlagBits::eIndexBuffer);

    if (device.createBuffer(&bufferInfo, nullptr, &indexBuffer) != vk::Result::eSuccess) {
        throw std::runtime_error("Failed to create index buffer!");
    }

    device.getBufferMemoryRequirements(indexBuffer, &memRequirements);

    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = ModelLoader::FindMemoryType(physicalDevice, memRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

    if (device.allocateMemory(&allocInfo, nullptr, &indexBufferMemory) != vk::Result::eSuccess) {
        throw std::runtime_error("Failed to allocate index buffer memory!");
    }

    device.bindBufferMemory(indexBuffer, indexBufferMemory, 0);

    device.mapMemory(indexBufferMemory, 0, VK_WHOLE_SIZE, {}, &data);
    memcpy(data, indices.data(), (size_t)indexBufferSize);
    device.unmapMemory(indexBufferMemory);
}



void MeshModel::setModelMatrix(glm::vec3 scale, glm::vec3 translation, glm::vec3 rotation) {

    glm::mat4 newModelMatrix = glm::mat4(1.0f);

    newModelMatrix = glm::scale(newModelMatrix, scale);

    newModelMatrix = glm::rotate(newModelMatrix, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));

    newModelMatrix = glm::rotate(newModelMatrix, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));

    newModelMatrix = glm::rotate(newModelMatrix, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

    newModelMatrix = glm::translate(newModelMatrix, translation);

    modelMatrix = newModelMatrix;
}


void MeshModel::cleanup(vk::Device& device, vk::PhysicalDevice& physicalDevice) {
    device.destroyBuffer(vertexBuffer);
    device.freeMemory(vertexBufferMemory);
    device.destroyBuffer(indexBuffer);
    device.freeMemory(indexBufferMemory);
}

glm::mat4* MeshModel::GetModelMat()
{
    return &modelMatrix;
}
