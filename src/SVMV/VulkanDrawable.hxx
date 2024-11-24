#pragma once

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

#include <cstdint>
#include <memory>

namespace SVMV
{
    struct MaterialInstance;

    struct VulkanDrawable
    {
        struct AttributeAddresses
        {
            vk::DeviceAddress positions;
            vk::DeviceAddress normals;
            vk::DeviceAddress tangents;
            vk::DeviceAddress texcoords_0;
            vk::DeviceAddress colors_0;
        };

        uint32_t firstIndex;
        uint32_t indexCount;

        AttributeAddresses addresses;

        glm::mat4 modelMatrix;

        std::shared_ptr<MaterialInstance> materialInstance;
    };
}
