#pragma once

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

#include <array>

struct VulkanVertex
{
    glm::vec2 position;
    glm::vec3 color;

    static vk::VertexInputBindingDescription getBindingDescription()
    {
        vk::VertexInputBindingDescription description = {};
        description.binding = 0;
        description.stride = sizeof(VulkanVertex);
        description.inputRate = vk::VertexInputRate::eVertex;

        return description;
    }

    static std::array<vk::VertexInputAttributeDescription, 2> getAttributeDescriptions()
    {
        std::array<vk::VertexInputAttributeDescription, 2> descriptions = {};

        descriptions[0].binding = 0;
        descriptions[0].location = 0;
        descriptions[0].format = vk::Format::eR32G32Sfloat;
        descriptions[0].offset = offsetof(VulkanVertex, position);
        
        descriptions[1].binding = 0;
        descriptions[1].location = 1;
        descriptions[1].format = vk::Format::eR32G32B32Sfloat;
        descriptions[1].offset = offsetof(VulkanVertex, color);

        return descriptions;
    }
};
