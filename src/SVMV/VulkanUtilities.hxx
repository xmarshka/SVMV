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

        class GLFWwindow
        {
        public:
            GLFWwindow() = default;
            GLFWwindow(const std::string& name, int width, int height, VulkanRenderer* rendererHandle, GLFWframebuffersizefun resizeCallback, GLFWwindowiconifyfun minimizedCallback);

            GLFWwindow(const GLFWwindow&) = delete;
            GLFWwindow& operator=(const GLFWwindow&) = delete;

            GLFWwindow(GLFWwindow&& other) noexcept;
            GLFWwindow& operator=(GLFWwindow&& other) noexcept;

            ~GLFWwindow();

            ::GLFWwindow* getWindow() const noexcept;
            vk::raii::SurfaceKHR getSurface(const vk::raii::Instance& instance) const noexcept;
        private:
            ::GLFWwindow* _window{ nullptr };
        };

        class VmaAllocator
        {
        public:
            VmaAllocator() = default;
            VmaAllocator(const vk::raii::Instance& instance, const vk::raii::PhysicalDevice& physicalDevice, const vk::raii::Device& device);

            VmaAllocator(const VmaAllocator&) = delete;
            VmaAllocator& operator=(const VmaAllocator&) = delete;

            VmaAllocator(VmaAllocator&& other) noexcept;
            VmaAllocator& operator=(VmaAllocator&& other) noexcept;

            ~VmaAllocator();

            ::VmaAllocator getAllocator() const noexcept;

        private:
            ::VmaAllocator _allocator{ nullptr };
        };
    }
}