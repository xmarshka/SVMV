#pragma once

#include <SVMV/VulkanBuffer.hxx>
#include <SVMV/VulkanUtilities.hxx>

namespace SVMV
{
    class VulkanImage
    {
    public:
        vk::Image image;
        vk::ImageView imageView;

        vk::Format format;
        vk::Extent3D extent;

        VmaAllocation allocation;
        VmaAllocationInfo info;

    private:
        vk::Device _device;
        VmaAllocator _allocator;
        VulkanUtilities::ImmediateSubmit _immediateSubmit;

    public:
        VulkanImage();
        void create(vk::Device device, VmaAllocator vmaAllocator, unsigned width, unsigned height, vk::Format format, vk::Flags<vk::ImageUsageFlagBits> imageUsage, VulkanUtilities::ImmediateSubmit immediateSubmit);
        void copyDataToImage(void* data, size_t dataSize);
        void free();

        operator bool() const;
    };
}