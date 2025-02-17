#pragma once

#include <vulkan/vulkan.hpp>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

namespace SVMV
{
    namespace ShaderStructures
    {
        struct PushConstants
        {
            vk::DeviceAddress positions         { 0 };
            vk::DeviceAddress normals           { 0 };
            vk::DeviceAddress tangents          { 0 };
            vk::DeviceAddress texcoords_0       { 0 };
            vk::DeviceAddress colors_0          { 0 };
            vk::DeviceAddress modelMatrix       { 0 };
            vk::DeviceAddress normalMatrix      { 0 };
        };

        struct GlobalUniformBuffer
        {
            glm::mat4 View              { 1.0f };
            glm::mat4 ViewProjection    { 1.0f };
            glm::vec4 CameraPosition    { 0.0f };

            // padding to align to 256 bytes
            glm::vec4 padding1;
            glm::vec4 padding2;
            glm::vec4 padding3;
            glm::mat4 padding4;
        };
    }
}