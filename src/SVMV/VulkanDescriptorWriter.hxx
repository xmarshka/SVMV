#pragma once

#include <vulkan/vulkan_raii.hpp>

#include <SVMV/VulkanBuffer.hxx>
#include <SVMV/VulkanImage.hxx>

namespace SVMV
{
    class VulkanDescriptorWriter
    {
    public:
        VulkanDescriptorWriter() = default;
        VulkanDescriptorWriter(vk::raii::Device* device);

        VulkanDescriptorWriter(const VulkanDescriptorWriter&) = delete;
        VulkanDescriptorWriter& operator=(const VulkanDescriptorWriter&) = delete;

        VulkanDescriptorWriter(VulkanDescriptorWriter&& other) noexcept;
        VulkanDescriptorWriter& operator=(VulkanDescriptorWriter&& other) noexcept;

        ~VulkanDescriptorWriter() = default;

        void writeBuffer(const vk::raii::DescriptorSet& descriptorSet, const VulkanBuffer& buffer, int binding, int offset, int size, vk::DescriptorType bufferType);
        void writeImage();
        void writeImageAndSampler(
            const vk::raii::DescriptorSet& descriptorSet, const VulkanImage& image,
            vk::ImageLayout imageLayout, const vk::raii::Sampler& sampler, int binding
        );

    private:
        vk::raii::Device* _device;
    };
}
