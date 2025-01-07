#pragma once

#include <SVMV/VulkanBuffer.hxx>
#include <SVMV/VulkanUtilities.hxx>

#include <memory>

namespace SVMV
{
    class VulkanImage
    {
    public:
        VulkanImage() = default;
        VulkanImage(
            vk::raii::Device* device, VulkanUtilities::ImmediateSubmit* immediateSubmit, VmaAllocator vmaAllocator,
            vk::Extent2D extent, vk::Format format, vk::ImageUsageFlags imageUsageFlags, void* data, size_t dataSize
        );

        VulkanImage(const VulkanImage&) = delete;
        VulkanImage& operator=(const VulkanImage&) = delete;

        VulkanImage(VulkanImage&& other) noexcept;
        VulkanImage& operator=(VulkanImage&& other) noexcept;

        ~VulkanImage();

        [[nodiscard]] const vk::raii::Image& getImage() const noexcept;
        [[nodiscard]] const vk::raii::ImageView& getImageView() const noexcept;
        [[nodiscard]] vk::Format getFormat() const noexcept;
        [[nodiscard]] vk::Extent3D getExtent() const noexcept;

        operator bool() const;

    private:
        void fillImage(void* data, size_t size);

    private:
        vk::raii::Image _image          { nullptr };
        vk::raii::ImageView _imageView  { nullptr };

        VmaAllocator _allocator         { nullptr };
        VmaAllocation _allocation       { nullptr };

        vk::raii::Device* _device                           { nullptr };
        VulkanUtilities::ImmediateSubmit* _immediateSubmit  { nullptr };

        vk::Format _format      { vk::Format::eUndefined };
        vk::Extent3D _extent    { 0 };
    };
}