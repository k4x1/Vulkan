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
    {
        Position = glm::vec3(0.0f);
        Normal = glm::vec3(0.0f);  
        TexCoord = glm::vec2(0.0f);
    };

    VertexStandard(glm::vec3 pos, glm::vec3 norm, glm::vec2 texc)
    {
        Position = pos;
        Normal = norm; 
        TexCoord = texc;
    }

    static vk::VertexInputBindingDescription GetBindingDescription()
    {
        vk::VertexInputBindingDescription BindingDescription{};

        BindingDescription.binding = 0;
        BindingDescription.stride = sizeof(VertexStandard);
        BindingDescription.inputRate = vk::VertexInputRate::eVertex;

        return BindingDescription;
    }

    static std::array<vk::VertexInputAttributeDescription, 3> GetAttributeDescriptions()
    {
        std::array<vk::VertexInputAttributeDescription, 3> AttributeDescriptions{};

        AttributeDescriptions[0].binding = 0;
        AttributeDescriptions[0].location = 0;
        AttributeDescriptions[0].format = vk::Format::eR32G32B32Sfloat;
        AttributeDescriptions[0].offset = offsetof(VertexStandard, Position);

        AttributeDescriptions[1].binding = 0;
        AttributeDescriptions[1].location = 1;
        AttributeDescriptions[1].format = vk::Format::eR32G32B32Sfloat;
        AttributeDescriptions[1].offset = offsetof(VertexStandard, Normal);  

        AttributeDescriptions[2].binding = 0;
        AttributeDescriptions[2].location = 2;
        AttributeDescriptions[2].format = vk::Format::eR32G32Sfloat;
        AttributeDescriptions[2].offset = offsetof(VertexStandard, TexCoord);

        return AttributeDescriptions;
    }
};
