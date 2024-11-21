#pragma once

#include <vulkan/vulkan.hpp>

#include <vector>

namespace SVMV
{
    class VulkanDescriptorAllocator
    {
    private:
        vk::Device _device;

        std::vector<vk::DescriptorPool> _availablePools;
        std::vector<vk::DescriptorPool> _filledPools;

        unsigned _setsPerPool; // increased for each new pool, similiar to a vector

    public:
        void initialize(vk::Device device);
        void destroyPools();
        void clearPools();

        vk::DescriptorSet allocateSet(vk::DescriptorSetLayout layout);

        vk::DescriptorPool getPool();
        vk::DescriptorPool createPool();
    };
}