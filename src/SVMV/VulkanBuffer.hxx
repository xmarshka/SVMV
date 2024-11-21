#pragma once

#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.h>

#include <memory>

namespace SVMV
{
    class VulkanBuffer
    {
    public:
        VmaAllocator allocator;

        vk::Buffer buffer;
        VmaAllocation allocation;
        VmaAllocationInfo info;

    public:
        VulkanBuffer();
        void create(VmaAllocator vmaAllocator, size_t bufferSize, vk::Flags<vk::BufferUsageFlagBits> bufferUsage, VmaMemoryUsage vmaMemoryUsage);
        void free();

        operator bool() const;
    };
}