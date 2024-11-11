#pragma once
#include "TinyObjConfig.h"
#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>

using namespace glm;

class Utils {
public:
    struct Vertex {
        vec3 pos;
        vec3 color;
        vec2 texCoord;

        static vk::VertexInputBindingDescription getBindingDescription();
        static std::array<vk::VertexInputAttributeDescription, 3> getAttributeDescriptions();
    };

    struct UniformBufferObject {
        mat4 model;
        mat4 view;
        mat4 proj;
    };

    template<typename T>
    static void Log(const T& message);

    static void Log(const vec2& vector);
    static void Log(const vec3& vector);
    static void Log(const vec4& vector);

    template<typename First, typename... Args>
    static void Log(const First& first, const Args&... args);

    static uint32_t findMemoryType(const vk::PhysicalDevice& physicalDevice, uint32_t typeFilter, vk::MemoryPropertyFlags properties);
    static void createBuffer(const vk::Device& device, const vk::PhysicalDevice& physicalDevice, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Buffer& buffer, vk::DeviceMemory& bufferMemory);
    static void copyBuffer(const vk::Device& device, const vk::Queue& queue, const vk::Buffer& srcBuffer, vk::Buffer& dstBuffer, vk::DeviceSize size);
    static vk::CommandBuffer beginSingleTimeCommands(const vk::Device& device, const vk::Queue& queue);
    static void endSingleTimeCommands(const vk::Device& device, const vk::Queue& queue, vk::CommandBuffer commandBuffer);

    static float getVectorLength(const vec2& vector);
    static vec2 normalize(vec2& vector);
    static vec2 lerp(vec2 point1, vec2 point2, float alpha);
    static vec2 quadLerp(vec2 point1, vec2 point2, vec2 point3, float alpha);
    static vec2 cubicLerp(vec2 point1, vec2 point2, vec2 point3, vec2 point4, float alpha);

private:
    static vk::CommandPool commandPool;
};
