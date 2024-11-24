#pragma once

#include <vulkan/vulkan.hpp>

#include <SVMV/VulkanBuffer.hxx>
#include <SVMV/VulkanMaterial.hxx>

#include <vector>
#include <memory>
#include <unordered_map>

namespace SVMV
{
    class Primitive;
    struct VulkanDrawable;

    struct VulkanDrawableCollection
    {
        struct AttributeBuffer
        {
            size_t count;

            VulkanBuffer buffer;
            vk::DeviceAddress bufferAddress;

            VulkanBuffer stagingBuffer;
            size_t dataOffset;
        };

        std::vector<VulkanDrawable> drawables;

        AttributeBuffer indices;

        AttributeBuffer positions;
        AttributeBuffer normals;
        AttributeBuffer tangents;
        AttributeBuffer texcoords_0;
        AttributeBuffer colors_0;

        MaterialPipeline materialPipeline;

        std::vector<vk::CommandBuffer> commandBuffers; // TODO: this shouldn't be here (???)
    };
}
