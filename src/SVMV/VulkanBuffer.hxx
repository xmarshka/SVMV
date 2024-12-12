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
        VulkanBuffer(vk::raii::Device* device, VmaAllocator vmaAllocator, size_t bufferSize, vk::Flags<vk::BufferUsageFlagBits> bufferUsage, VmaMemoryUsage vmaMemoryUsage, bool createMapped = false);

        VulkanBuffer(const VulkanBuffer&) = delete;
        VulkanBuffer& operator=(const VulkanBuffer&) = delete;

        VulkanBuffer(VulkanBuffer&& other) noexcept;
        VulkanBuffer& operator=(VulkanBuffer&& other) noexcept;

        ~VulkanBuffer();

        operator bool() const;

        [[nodiscard]] const vk::raii::Buffer& getBuffer() const;
        [[nodiscard]] const VmaAllocator getAllocator() const;
        [[nodiscard]] const VmaAllocation getAllocation() const;

    protected:
        vk::raii::Buffer _buffer{ nullptr };

        VmaAllocator _allocator{ nullptr };
        VmaAllocation _allocation{ nullptr };
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
        uint8_t* _mappedData;

        VulkanUtilities::ImmediateSubmit* _immediateSubmit{ nullptr };
        size_t _capacity{ 0 };
        size_t _filledSize{ 0 };
    };
}