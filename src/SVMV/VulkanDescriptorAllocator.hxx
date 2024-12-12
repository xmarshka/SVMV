#pragma once

#include <vulkan/vulkan_raii.hpp>

#include <vector>
#include <memory>

namespace SVMV
{
    class VulkanDescriptorAllocator
    {
    public:
        VulkanDescriptorAllocator() = default;
        VulkanDescriptorAllocator(vk::raii::Device* device);

        VulkanDescriptorAllocator(const VulkanDescriptorAllocator&) = delete;
        VulkanDescriptorAllocator& operator=(const VulkanDescriptorAllocator&) = delete;

        VulkanDescriptorAllocator(VulkanDescriptorAllocator&& other) noexcept;
        VulkanDescriptorAllocator& operator=(VulkanDescriptorAllocator&& other) noexcept;

        ~VulkanDescriptorAllocator() = default;

        void destroyPools();
        void clearPools();

        vk::raii::DescriptorSet allocateSet(const vk::raii::DescriptorSetLayout& layout);

    private:
        std::shared_ptr<vk::raii::DescriptorPool> getPool();
        std::shared_ptr<vk::raii::DescriptorPool> createPool();

    private:
        vk::raii::Device* _device{ nullptr };

        std::vector<std::shared_ptr<vk::raii::DescriptorPool>> _availablePools;
        std::vector<std::shared_ptr<vk::raii::DescriptorPool>> _filledPools;

        unsigned _setsPerPool{ 8 }; // increased for each new pool, similiar to a vector

    };
}
