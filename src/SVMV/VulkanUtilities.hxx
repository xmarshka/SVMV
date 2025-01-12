#pragma once

#include <vulkan/vulkan_raii.hpp>
#include <GLFW/glfw3.h>
#include <vk_mem_alloc.h>

#include <functional>

namespace SVMV
{
    class VulkanRenderer;

    namespace VulkanUtilities
    {
        class ImmediateSubmit
        {
        public:
            ImmediateSubmit() = default;
            ImmediateSubmit(vk::raii::Device* device, vk::raii::Queue* queue, unsigned queueFamily);

            ImmediateSubmit(const ImmediateSubmit&) = delete;
            ImmediateSubmit& operator=(const ImmediateSubmit&) = delete;

            ImmediateSubmit(ImmediateSubmit&& other) noexcept;
            ImmediateSubmit& operator=(ImmediateSubmit&& other) noexcept;

            ~ImmediateSubmit() = default;

            void submit(std::function<void(const vk::raii::CommandBuffer& commandBuffer)>&& lambda);

        private:
            vk::raii::Device* _device{ nullptr };
            vk::raii::Queue* _queue{ nullptr };

            vk::raii::CommandPool _commandPool{ nullptr };
            vk::raii::CommandBuffer _commandBuffer{ nullptr };
            vk::raii::Fence _fence{ nullptr };
        };

        class DescriptorAllocator
        {
        public:
            DescriptorAllocator() = default;
            DescriptorAllocator(vk::raii::Device* device);

            DescriptorAllocator(const DescriptorAllocator&) = delete;
            DescriptorAllocator& operator=(const DescriptorAllocator&) = delete;

            DescriptorAllocator(DescriptorAllocator&& other) noexcept;
            DescriptorAllocator& operator=(DescriptorAllocator&& other) noexcept;

            ~DescriptorAllocator() = default;

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

        class VmaAllocatorWrapper
        {
        public:
            VmaAllocatorWrapper() = default;
            VmaAllocatorWrapper(const vk::raii::Instance& instance, const vk::raii::PhysicalDevice& physicalDevice, const vk::raii::Device& device);

            VmaAllocatorWrapper(const VmaAllocatorWrapper&) = delete;
            VmaAllocatorWrapper& operator=(const VmaAllocatorWrapper&) = delete;

            VmaAllocatorWrapper(VmaAllocatorWrapper&& other) noexcept;
            VmaAllocatorWrapper& operator=(VmaAllocatorWrapper&& other) noexcept;

            ~VmaAllocatorWrapper();

            VmaAllocator getAllocator() const noexcept;

        private:
            VmaAllocator _allocator{ nullptr };
        };

        vk::raii::Pipeline createPipeline(const vk::raii::Device& device, const vk::raii::PipelineLayout& pipelineLayout, const vk::raii::RenderPass& renderPass, const vk::raii::ShaderModule& vertexShader, const vk::raii::ShaderModule& fragmentShader);
    }
}