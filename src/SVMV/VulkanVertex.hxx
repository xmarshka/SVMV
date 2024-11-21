#pragma once

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

struct VulkanVertex
{
    glm::vec3 position;
    float texcoordX;
    glm::vec3 normal;
    float texcoordY;
    glm::vec4 tangent;
    glm::vec4 color;
};
