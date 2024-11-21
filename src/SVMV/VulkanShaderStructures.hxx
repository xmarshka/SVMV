#pragma once

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

namespace SVMV
{
    namespace ShaderStructures
    {
        struct PushConstants
        {
            glm::mat4 mvpMatrix;
            vk::DeviceAddress vertexBufferAddress;
        };

        struct VertexAttributeAddresses
        {
            vk::DeviceAddress positions;
            vk::DeviceAddress normals;
            vk::DeviceAddress tangents;
            vk::DeviceAddress texcoords;
            vk::DeviceAddress colors;
        };

        struct MaterialParameters
        {

        };
    }
}