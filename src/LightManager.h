#pragma once

#include "common.h"
#include <vector>

struct PointLight {
    glm::vec3 position;
    float intensity;
    glm::vec3 color;
    float constant;
    float linear; 
    float quadratic;
    float paddington1;
    float paddington2;
};


struct PointLightBufferData {
    PointLight lights[10]; 
    int lightCount = 0; 
};

class LightManager {
public:
    static LightManager& GetInstance() {
        static LightManager instance;
        return instance;
    }

    LightManager(const LightManager&) = delete;
    LightManager& operator=(const LightManager&) = delete;

    void Init(vk::Device& device, vk::PhysicalDevice& gpu);
    void createPointLight(const PointLight& newPointLight);
    void updatePointLightBuffer(vk::Device& device);
    vk::Buffer getPointLightBuffer() const;
    void CleanUp(vk::Device& device);
    
    static const int MAX_POINT_LIGHTS = 10;

protected:
    LightManager() = default;
    ~LightManager() = default;

private:
    void createPointLightBuffer(vk::Device& device, vk::PhysicalDevice& gpu);

    PointLight pointLights[MAX_POINT_LIGHTS] = {};
    uint32_t pointLightCount = 0;

    vk::Buffer pointLightBuffer;
    vk::DeviceMemory pointLightMemory;
};
