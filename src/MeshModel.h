#pragma once
#include "common.h"
#include "VertexStandard.h"
#include "ModelLoader.h"
#include <vector>


class MeshModel {
public:
    MeshModel() = default;
      

    void loadModel(const std::string& filepath);
    void render(const vk::CommandBuffer& commandBuffer);
    void updateUniformData(glm::mat4 viewProjMatrix, void* uniformMemoryPtr);
    void loadVBO(vk::Device& device, vk::PhysicalDevice& physicalDevice);
    void setModelMatrix(glm::vec3 scale, glm::vec3 translation, glm::vec3 rotation);
    void cleanup(vk::Device& device, vk::PhysicalDevice& physicalDevice);
    glm::mat4* GetModelMat();

private:
    vk::Device* device;
    vk::PhysicalDevice* physicalDevice;
    vk::Buffer vertexBuffer;
    vk::DeviceMemory vertexBufferMemory;
    std::vector<VertexStandard> vertices;
    glm::mat4 modelMatrix;
};
