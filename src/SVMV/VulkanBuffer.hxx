#pragma once

#include <SVMV/VulkanUtilities.hxx>

#include <vulkan/vulkan_raii.hpp>
#include <vk_mem_alloc.h>

#include <memory>

namespace SVMV
{
    class VulkanBuffer
    {
    public:
        VulkanBuffer() = default;
        VulkanBuffer(vk::raii::Device* device, VmaAllocator vmaAllocator, size_t bufferSize, vk::Flags<vk::BufferUsageFlagBits> bufferUsage, bool enableWriting = false);

        VulkanBuffer(const VulkanBuffer&) = delete;
        VulkanBuffer& operator=(const VulkanBuffer&) = delete;

        VulkanBuffer(VulkanBuffer&& other) noexcept;
        VulkanBuffer& operator=(VulkanBuffer&& other) noexcept;

        ~VulkanBuffer();

        operator bool() const;

        [[nodiscard]] const vk::raii::Buffer& getBuffer() const; // TODO: noexcept
        [[nodiscard]] const VmaAllocator getAllocator() const;
        [[nodiscard]] const VmaAllocation getAllocation() const;
        [[nodiscard]] const size_t getSize() const;
        [[nodiscard]] const vk::DeviceAddress getAddress(const vk::Device& device) const;

    protected:
        vk::raii::Buffer _buffer{ nullptr };

        VmaAllocator _allocator{ nullptr };
        VmaAllocation _allocation{ nullptr };

        size_t _size{ 0 };
    };

    // GPU BUFFER

    class VulkanGPUBuffer : public VulkanBuffer
    {
    public:
        VulkanGPUBuffer() = default;
        VulkanGPUBuffer(vk::raii::Device* device, VmaAllocator vmaAllocator, size_t bufferSize, vk::Flags<vk::BufferUsageFlagBits> bufferUsage);

        VulkanGPUBuffer(const VulkanGPUBuffer&) = delete;
        VulkanGPUBuffer& operator=(const VulkanGPUBuffer&) = delete;

        VulkanGPUBuffer(VulkanGPUBuffer&& other) noexcept;
        VulkanGPUBuffer& operator=(VulkanGPUBuffer&& other) noexcept;

        ~VulkanGPUBuffer() = default;
    };

    // STAGING BUFFER

    class VulkanStagingBuffer : public VulkanBuffer
    {
    public:
        VulkanStagingBuffer() = default;
        VulkanStagingBuffer(vk::raii::Device* device, VulkanUtilities::ImmediateSubmit* immediateSubmit, VmaAllocator vmaAllocator, size_t bufferSize);
        VulkanStagingBuffer(vk::raii::Device* device, const VulkanBuffer& destinationBuffer, VulkanUtilities::ImmediateSubmit* immediateSubmit);

        VulkanStagingBuffer(const VulkanStagingBuffer&) = delete;
        VulkanStagingBuffer& operator=(const VulkanStagingBuffer&) = delete;

        VulkanStagingBuffer(VulkanStagingBuffer&& other) noexcept;
        VulkanStagingBuffer& operator=(VulkanStagingBuffer&& other) noexcept;

        ~VulkanStagingBuffer();

        void pushData(void* data, size_t size);

        void copyToBuffer(const VulkanBuffer& destination);
        void copyToBuffer(const VulkanBuffer& destination, size_t sizeToCopy, size_t offset = 0);

    private:
        std::byte* _mappedData{ nullptr };

        VulkanUtilities::ImmediateSubmit* _immediateSubmit{ nullptr };
        size_t _capacity{ 0 };
        size_t _filledSize{ 0 };
    };
}