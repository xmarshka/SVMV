#pragma once

#include <SVMV/VulkanBuffer.hxx>

namespace SVMV
{
    class VulkanImage
    {
    public:
        vk::Device device;
        VmaAllocator allocator;

        vk::Image image;
        vk::ImageView imageView;

        vk::Format format;
        vk::Extent3D extent;

        VmaAllocation allocation;
        VmaAllocationInfo info;

    public:
        VulkanImage();
        void create(vk::Device device, VmaAllocator vmaAllocator, unsigned width, unsigned height, vk::Format format, vk::Flags<vk::ImageUsageFlagBits> imageUsage);
        void copyDataToImage(void* data, size_t dataSize, vk::CommandBuffer commandBuffer, vk::Queue queue);
        void free();

        operator bool() const;
    };
}