#pragma once

#include <SVMV/VulkanBuffer.hxx>
#include <SVMV/VulkanUtilities.hxx>

#include <memory>

namespace SVMV
{
    class VulkanImage
    {
    public:
        vk::raii::Image image;
        vk::raii::ImageView imageView;

        vk::Format format;
        vk::Extent3D extent;

        VmaAllocation allocation;
        VmaAllocationInfo info;

    private:
        vk::raii::Device* _device;
        VmaAllocator _allocator;
        VulkanUtilities::ImmediateSubmit* _immediateSubmit;

    public:
        VulkanImage(const VulkanImage&) = delete;
        VulkanImage& operator=(const VulkanImage&) = delete;

        VulkanImage();
        VulkanImage(vk::raii::Device* device, VmaAllocator vmaAllocator, unsigned width, unsigned height, vk::Format format, vk::Flags<vk::ImageUsageFlagBits> imageUsage, VulkanUtilities::ImmediateSubmit* immediateSubmit);
        ~VulkanImage();

        void create(vk::raii::Device* device, VmaAllocator vmaAllocator, unsigned width, unsigned height, vk::Format format, vk::Flags<vk::ImageUsageFlagBits> imageUsage, VulkanUtilities::ImmediateSubmit* immediateSubmit);
        void copyDataToImage(void* data, size_t dataSize);
        void free();

        operator bool() const;
    };
}