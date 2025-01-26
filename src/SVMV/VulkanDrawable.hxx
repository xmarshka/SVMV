#pragma once

#include <vulkan/vulkan.hpp>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <cstdint>
#include <memory>

#include <SVMV/Attribute.hxx>

namespace SVMV
{
    struct AttributeAddresses
    {
        vk::DeviceAddress positions         { 0 };
        vk::DeviceAddress normals           { 0 };
        vk::DeviceAddress tangents          { 0 };
        vk::DeviceAddress texcoords_0       { 0 };
        vk::DeviceAddress colors_0          { 0 };
    };

    struct VulkanDrawable
    {
        uint32_t firstIndex     { 0 };
        uint32_t indexCount     { 0 };

        vk::DeviceAddress modelMatrixAddress        { 0 };
        AttributeAddresses attributeAddresses;

        vk::raii::DescriptorSet* descriptorSet      { nullptr };

        void setAddress(AttributeType type, vk::DeviceAddress value)
        {
            switch (type)
            {
            case AttributeType::POSITION:
                attributeAddresses.positions = value;
                break;
            case AttributeType::NORMAL:
                attributeAddresses.normals = value;
                break;
            case AttributeType::TANGENT:
                attributeAddresses.tangents = value;
                break;
            case AttributeType::TEXCOORD_0:
                attributeAddresses.texcoords_0 = value;
                break;
            case AttributeType::COLOR_0:
                attributeAddresses.colors_0 = value;
                break;
            default:
                return;
            }
        }
    };
}
