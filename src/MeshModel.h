#pragma once

#include <iostream>
#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "VertexStandard.h"
#include "Camera.h"
#include "Utils.h"
//#include "Light.h"

class MeshModel {
public:
    MeshModel(vk::Device device, vk::PhysicalDevice physicalDevice, vk::CommandPool commandPool, vk::Queue graphicsQueue,
        glm::vec3 position = glm::vec3(0), glm::vec3 rotation = glm::vec3(0), glm::vec3 scale = glm::vec3(1),
        const std::string& modelFilePath = "");
    ~MeshModel();

    void Update(float deltaTime);
    void Render(vk::CommandBuffer commandBuffer, vk::Pipeline pipeline, vk::PipelineLayout pipelineLayout, vk::DescriptorSet descriptorSet);

    void SetPosition(const glm::vec3& newPos);
    void SetRotation(const glm::vec3& newRot);
    void SetScale(const glm::vec3& newScale);

    void SetTexture(vk::ImageView textureImageView, vk::Sampler textureSampler);
  //  void SetMaterial(const Material& material);

    void UpdateUniformBuffer(vk::DeviceMemory uniformMemory, const Utils::UniformBufferObject& ubo);
    void CreateVertexBuffer(const std::vector<Utils::Vertex>& vertices);

private:
    vk::Device m_device;
    vk::PhysicalDevice m_physicalDevice;
    vk::CommandPool m_commandPool;
    vk::Queue m_graphicsQueue;

    vk::Buffer m_vertexBuffer;
    vk::DeviceMemory m_vertexBufferMemory;
    uint32_t m_vertexCount;

    vk::Buffer m_indexBuffer;
    vk::DeviceMemory m_indexBufferMemory;
    uint32_t m_indexCount;

    vk::ImageView m_textureImageView;
    vk::Sampler m_textureSampler;

 //   Material m_material;
    vk::PipelineLayout m_pipelineLayout;

    glm::vec3 m_position;
    glm::vec3 m_rotation;
    glm::vec3 m_scale;
    glm::mat4 m_modelMatrix;

    void LoadModel(const std::string& modelFilePath);
    void CreateVertexBuffer(const std::vector<VertexStandard>& vertices);
    void CreateIndexBuffer(const std::vector<uint32_t>& indices);
    glm::mat4 CalculateModelMatrix();
};
