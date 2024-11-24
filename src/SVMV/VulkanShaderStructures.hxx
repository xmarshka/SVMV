#pragma once

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

namespace SVMV
{
    namespace ShaderStructures
    {
        struct VertexAttributeAddresses
        {
            vk::DeviceAddress positions;
            vk::DeviceAddress normals;
            vk::DeviceAddress tangents;
            vk::DeviceAddress texcoords_0;
            vk::DeviceAddress colors_0;
        };

        struct PushConstants
        {
            glm::mat4 mvpMatrix;
            VertexAttributeAddresses addresses;
        };
    }
}