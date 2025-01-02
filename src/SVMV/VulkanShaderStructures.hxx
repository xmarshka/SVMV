#pragma once

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

namespace SVMV
{
    namespace ShaderStructures
    {
        struct PushConstants
        {
            vk::DeviceAddress positions     { 0 };
            vk::DeviceAddress normals       { 0 };
            vk::DeviceAddress tangents      { 0 };
            vk::DeviceAddress texcoords_0   { 0 };
            vk::DeviceAddress colors_0      { 0 };
            vk::DeviceAddress modelMatrix   { 0 };
        };

        struct GlobalUniformBuffer
        {
            glm::mat4 View             { 1.0f };
            glm::mat4 ViewProjection   { 1.0f };

            // padding to align to 256 bytes
            glm::mat4 padding;
        };
    }
}