#pragma once

#include <vulkan/vulkan_raii.hpp>
#include <VkBootstrap.h>

#include <vector>
#include <array>
#include <utility>

namespace SVMV
{
    class VulkanInitilization
    {
    public:
        VulkanInitilization() = default;

        VulkanInitilization(const VulkanInitilization&) = delete;
        VulkanInitilization& operator=(const VulkanInitilization&) = delete;

        VulkanInitilization(VulkanInitilization&& other) = delete;
        VulkanInitilization& operator=(VulkanInitilization&& other) = delete;

        ~VulkanInitilization() = default;

        vk::raii::Instance createInstance(const vk::raii::Context& context, const std::string& name, unsigned apiVersionMajor, unsigned apiVersionMinor);
        vk::raii::DebugUtilsMessengerEXT createDebugMessenger(const vk::raii::Instance& instance);
        vk::raii::PhysicalDevice createPhysicalDevice(const vk::raii::Instance& instance, std::vector<const char*> extensions, const vk::raii::SurfaceKHR& surface);
        vk::raii::Device createDevice(const vk::raii::PhysicalDevice& physicalDevice);
        vk::raii::SwapchainKHR createSwapchain(const vk::raii::Device& device, const vk::raii::SurfaceKHR& surface);
        vk::raii::SwapchainKHR recreateSwapchain(const vk::raii::Device& device, const vk::raii::SurfaceKHR& surface);
        vk::raii::CommandPool createCommandPool(const vk::raii::Device& device);
        std::pair<vk::raii::Queue, unsigned> createQueue(const vk::raii::Device& device, vkb::QueueType queueType);
        std::vector<vk::raii::ImageView> createSwapchainImageViews(const vk::raii::Device& device);
        std::vector<vk::raii::Framebuffer> createFramebuffers(const vk::raii::Device& device, const vk::raii::RenderPass& renderPass, const std::vector<vk::raii::ImageView>& imageViews);
        std::vector<vk::raii::Framebuffer> createFramebuffers(const vk::raii::Device& device, const vk::raii::RenderPass& renderPass, const std::vector<vk::raii::ImageView>& imageViews, const vk::raii::ImageView& depthImageView);
        std::vector<vk::raii::Semaphore> createSemaphores(const vk::raii::Device& device, int count);
        std::vector<vk::raii::Fence> createFences(const vk::raii::Device& device, int count);

        vk::Extent2D getSwapchainExtent();
        vk::Format getSwapchainFormat();

    private:
        vkb::Instance _bootstrapInstance;
        vkb::PhysicalDevice _bootstrapPhysicalDevice;
        vkb::Device _bootstrapDevice;
        vkb::Swapchain _bootstrapSwapchain;
    };
}
