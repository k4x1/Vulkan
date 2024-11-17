#include "LightManager.h"
#include "ModelLoader.h"

void LightManager::Init(vk::Device& device, vk::PhysicalDevice& gpu) {
    createPointLightBuffer(device, gpu);
}

void LightManager::createPointLight(const PointLight& newPointLight) {
    if (pointLightCount < MAX_POINT_LIGHTS) {
        pointLights[pointLightCount] = newPointLight;
        pointLightCount++;
    }
}

void LightManager::createPointLightBuffer(vk::Device& device, vk::PhysicalDevice& gpu) {
    VkDeviceSize bufferSize = sizeof(PointLightBufferData);

    vk::BufferCreateInfo bufferInfo = {};
    bufferInfo.setSize(bufferSize)
        .setUsage(vk::BufferUsageFlagBits::eUniformBuffer)
        .setSharingMode(vk::SharingMode::eExclusive);

    auto result = device.createBuffer(&bufferInfo, nullptr, &pointLightBuffer);
    VERIFY(result == vk::Result::eSuccess);

    vk::MemoryRequirements memRequirements;
    device.getBufferMemoryRequirements(pointLightBuffer, &memRequirements);

    vk::MemoryAllocateInfo allocInfo = {};
    allocInfo.setAllocationSize(memRequirements.size);
    allocInfo.setMemoryTypeIndex(ModelLoader::FindMemoryType(gpu, memRequirements.memoryTypeBits,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent));

    result = device.allocateMemory(&allocInfo, nullptr, &pointLightMemory);
    VERIFY(result == vk::Result::eSuccess);

    result = device.bindBufferMemory(pointLightBuffer, pointLightMemory, 0);
    VERIFY(result == vk::Result::eSuccess);
}

void LightManager::updatePointLightBuffer(vk::Device& device) {
    PointLightBufferData bufferData;
    memcpy(bufferData.lights, pointLights, sizeof(PointLight) * pointLightCount);
    bufferData.lightCount = pointLightCount;
    void* data;
    auto result = device.mapMemory(pointLightMemory, 0, sizeof(PointLightBufferData), vk::MemoryMapFlags(), &data);
    VERIFY(result == vk::Result::eSuccess);

    memcpy(data, &bufferData, sizeof(PointLightBufferData));
    device.unmapMemory(pointLightMemory);
}

vk::Buffer LightManager::getPointLightBuffer() const {
    return pointLightBuffer;
}

void LightManager::CleanUp(vk::Device& device) {
    device.destroyBuffer(pointLightBuffer);
    device.freeMemory(pointLightMemory);
}
