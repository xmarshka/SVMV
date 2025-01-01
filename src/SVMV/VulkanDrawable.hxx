#pragma once

#include <SVMV/Attribute.hxx>

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

#include <cstdint>
#include <memory>

namespace SVMV
{
    struct AttributeAddresses
    {
        vk::DeviceAddress positions{ 0 };
        vk::DeviceAddress normals{ 0 };
        vk::DeviceAddress tangents{ 0 };
        vk::DeviceAddress texcoords_0{ 0 };
        vk::DeviceAddress colors_0{ 0 };
    };

    struct VulkanDrawable
    {
        uint32_t firstIndex;
        uint32_t indexCount;

        AttributeAddresses addresses;

        glm::mat4 modelMatrix{ 1.0f };

        std::shared_ptr<vk::raii::DescriptorSet> descriptorSet;

        void setAddress(AttributeType type, vk::DeviceAddress value)
        {
            switch (type)
            {
            case AttributeType::POSITION:
                addresses.positions = value;
                break;
            case AttributeType::NORMAL:
                addresses.normals = value;
                break;
            case AttributeType::TANGENT:
                addresses.tangents = value;
                break;
            case AttributeType::TEXCOORD_0:
                addresses.texcoords_0 = value;
                break;
            case AttributeType::COLOR_0:
                addresses.colors_0 = value;
                break;
            default:
                return;
            }
        }
    };
}
