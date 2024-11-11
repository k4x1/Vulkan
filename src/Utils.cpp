#include "utils.h"

vk::VertexInputBindingDescription Utils::Vertex::getBindingDescription() {
    return vk::VertexInputBindingDescription(0, sizeof(Vertex), vk::VertexInputRate::eVertex);
}

std::array<vk::VertexInputAttributeDescription, 3> Utils::Vertex::getAttributeDescriptions() {
    return {
        vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, pos)),
        vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color)),
        vk::VertexInputAttributeDescription(2, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, texCoord))
    };
}

template<typename T>
void Utils::Log(const T& message) {
    std::cout << message << std::endl;
}

void Utils::Log(const vec2& vector) {
    std::cout << "(" << vector.x << ", " << vector.y << ")" << std::endl;
}

void Utils::Log(const vec3& vector) {
    std::cout << "(" << vector.x << ", " << vector.y << ", " << vector.z << ")" << std::endl;
}

void Utils::Log(const vec4& vector) {
    std::cout << "(" << vector.x << ", " << vector.y << ", " << vector.z << ", " << vector.w << ")" << std::endl;
}

template<typename First, typename... Args>
void Utils::Log(const First& first, const Args&... args) {
    Log(first);
    Log(args...);
}

uint32_t Utils::findMemoryType(const vk::PhysicalDevice& physicalDevice, uint32_t typeFilter, vk::MemoryPropertyFlags properties) {
    vk::PhysicalDeviceMemoryProperties memProperties;
    physicalDevice.getMemoryProperties(&memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}

void Utils::createBuffer(const vk::Device& device, const vk::PhysicalDevice& physicalDevice, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Buffer& buffer, vk::DeviceMemory& bufferMemory) {
    vk::BufferCreateInfo bufferInfo({}, size, usage, vk::SharingMode::eExclusive);

    if (device.createBuffer(&bufferInfo, nullptr, &buffer) != vk::Result::eSuccess) {
        throw std::runtime_error("failed to create buffer!");
    }

    vk::MemoryRequirements memRequirements;
    device.getBufferMemoryRequirements(buffer, &memRequirements);

    vk::MemoryAllocateInfo allocInfo(memRequirements.size, findMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties));

    if (device.allocateMemory(&allocInfo, nullptr, &bufferMemory) != vk::Result::eSuccess) {
        throw std::runtime_error("failed to allocate buffer memory!");
    }

    device.bindBufferMemory(buffer, bufferMemory, 0);
}

void Utils::copyBuffer(const vk::Device& device, const vk::Queue& queue, const vk::Buffer& srcBuffer, vk::Buffer& dstBuffer, vk::DeviceSize size) {
    vk::CommandBuffer commandBuffer = beginSingleTimeCommands(device, queue);

    vk::BufferCopy copyRegion(0, 0, size);
    commandBuffer.copyBuffer(srcBuffer, dstBuffer, 1, &copyRegion);

    endSingleTimeCommands(device, queue, commandBuffer);
}

vk::CommandBuffer Utils::beginSingleTimeCommands(const vk::Device& device, const vk::Queue& queue) {
    vk::CommandBuffer commandBuffer;

    vk::CommandBufferAllocateInfo allocInfo(commandPool, vk::CommandBufferLevel::ePrimary, 1);

    device.allocateCommandBuffers(&allocInfo, &commandBuffer);

    vk::CommandBufferBeginInfo beginInfo;
    commandBuffer.begin(&beginInfo);

    return commandBuffer;
}

void Utils::endSingleTimeCommands(const vk::Device& device, const vk::Queue& queue, vk::CommandBuffer commandBuffer) {
    commandBuffer.end();

    vk::SubmitInfo submitInfo;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    queue.submit(1, &submitInfo, vk::Fence());

    queue.waitIdle();

    vkFreeCommandBuffers(device, commandPool, 1, reinterpret_cast<VkCommandBuffer*>(&commandBuffer));
}

float Utils::getVectorLength(const vec2& vector) {
    return sqrt((vector.x * vector.x) + (vector.y * vector.y));
}

vec2 Utils::normalize(vec2& vector) {
    float magnitude = getVectorLength(vector);
    return vector /= magnitude;
}

vec2 Utils::lerp(vec2 point1, vec2 point2, float alpha) {
    return point1 + (point2 - point1) * alpha;
}

vec2 Utils::quadLerp(vec2 point1, vec2 point2, vec2 point3, float alpha) {
    vec2 vecA = lerp(point1, point2, alpha);
    vec2 vecB = lerp(point2, point3, alpha);
    return lerp(vecA, vecB, alpha);
}

vec2 Utils::cubicLerp(vec2 point1, vec2 point2, vec2 point3, vec2 point4, float alpha) {
    vec2 vecA = quadLerp(point1, point2, point3, alpha);
    vec2 vecB = quadLerp(point2, point3, point4, alpha);
    return lerp(vecA, vecB, alpha);
}

vk::CommandPool Utils::commandPool;
