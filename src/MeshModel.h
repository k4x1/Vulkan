#pragma once

#include <vulkan/vulkan.hpp>
#include "VertexStandard.h"
#include "ModelLoader.h"
#include <vector>
#include <glm/glm.hpp>

class MeshModel {
public:
    MeshModel() = default;
      

    void loadModel(const std::string& filepath, vk::Device& device, vk::PhysicalDevice& physicalDevice);
    void render(const vk::CommandBuffer& commandBuffer);
    void updateUniformData(glm::mat4 modelMatrix, glm::mat4 viewProjMatrix, void* uniformMemoryPtr);

    void cleanup(vk::Device& device, vk::PhysicalDevice& physicalDevice);

private:
    vk::Device* device;
    vk::PhysicalDevice* physicalDevice;
    vk::Buffer vertexBuffer;
    vk::DeviceMemory vertexBufferMemory;
    std::vector<VertexStandard> vertices;
    glm::mat4 modelMatrix;
};
