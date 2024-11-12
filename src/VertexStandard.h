#pragma once
#include <glm/glm.hpp>
#include <array>
#include <vulkan/vulkan.hpp>

struct VertexStandard {
public:
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoord;

    VertexStandard()
        : Position(0.0f), Normal(0.0f), TexCoord(0.0f) {}

    VertexStandard(glm::vec3 pos, glm::vec3 norm, glm::vec2 texc)
        : Position(pos), Normal(norm), TexCoord(texc) {}

    static vk::VertexInputBindingDescription GetBindingDescription() {
        vk::VertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(VertexStandard);
        bindingDescription.inputRate = vk::VertexInputRate::eVertex;
        return bindingDescription;
    }

    static std::array<vk::VertexInputAttributeDescription, 3> GetAttributeDescriptions() {
        std::array<vk::VertexInputAttributeDescription, 3> attributeDescriptions{};
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = vk::Format::eR32G32B32Sfloat;
        attributeDescriptions[0].offset = offsetof(VertexStandard, Position);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = vk::Format::eR32G32B32Sfloat;
        attributeDescriptions[1].offset = offsetof(VertexStandard, Normal);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = vk::Format::eR32G32Sfloat;
        attributeDescriptions[2].offset = offsetof(VertexStandard, TexCoord);

        return attributeDescriptions;
    }

};
