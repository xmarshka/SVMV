#pragma once

#include <SVMV/Primitive.hxx>
#include <SVMV/VulkanBuffer.hxx>

namespace SVMV
{
    class AttributeBuffer
    {
        struct Attribute
        {
            VulkanBuffer buffer;
        };
    private:
        size_t count;

        VulkanBuffer buffer;
        vk::DeviceAddress bufferAddress;

        VulkanBuffer stagingBuffer;
        size_t dataOffset;
    };
}
