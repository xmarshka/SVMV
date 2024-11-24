#pragma once

#include <vulkan\vulkan.hpp>

#include <functional>

namespace SVMV
{
    namespace VulkanUtilities
    {
        class ImmediateSubmit
        {
        private:
            vk::Device _device;
            vk::Queue _queue;

            vk::CommandPool _pool;
            vk::CommandBuffer _buffer;
            vk::Fence _fence;

        public:
            void initialize(vk::Device device, vk::Queue queue, unsigned queueFamily);
            void free();

            void submit(std::function<void(vk::CommandBuffer buffer)>&& lambda);
        };
    }
}