#pragma once

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

namespace SVMV
{
    namespace ShaderStructures
    {
        struct VertexAttributeAddresses
        {
            vk::DeviceAddress positions{ 0 };
            vk::DeviceAddress normals{ 0 };
            vk::DeviceAddress tangents{ 0 };
            vk::DeviceAddress texcoords_0{ 0 };
            vk::DeviceAddress colors_0{ 0 };
        };

        struct PushConstants
        {
            glm::mat4 mvpMatrix{ 1.0f };
            VertexAttributeAddresses addresses;
        };
    }
}