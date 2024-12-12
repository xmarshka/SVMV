#pragma once

#include <vulkan/vulkan_raii.hpp>
#include <vk_mem_alloc.h>
#include <VkBootstrap.h>
#include <shaderc/shaderc.hpp>

#include <GLFW/glfw3.h>

#include <SVMV/Scene.hxx>
#include <SVMV/VulkanInitialization.hxx>
#include <SVMV/VulkanScene.hxx>

#include <memory>
#include <vector>
#include <fstream>
#include <iostream>
#include <sstream>

namespace SVMV
{
    class VulkanRenderer
    {
    public:
        VulkanRenderer() = default;
        VulkanRenderer(int width, int height, const std::string& name, unsigned framesInFlight);

        VulkanRenderer(const VulkanRenderer&) = delete;
        VulkanRenderer& operator=(const VulkanRenderer&) = delete;

        VulkanRenderer(VulkanRenderer&&) = delete;
        VulkanRenderer& operator=(VulkanRenderer&&) = delete;

        ~VulkanRenderer() = default;

        void draw();
        void loadScene(std::shared_ptr<Scene> scene);

        [[nodiscard]] const vk::Device getDevice() const noexcept;
        [[nodiscard]] const GLFWwindow* getWindow() const noexcept;

    private:
        void recreateSwapchain();
        void createRenderPass();

        static void resized(GLFWwindow* window, int width, int height);
        static void minimized(GLFWwindow* window, int minimized);

    private:
        VulkanUtilities::GLFWwindow _window;

        bool _resized                               { false };
        unsigned _framesInFlight                    { 0 };
        unsigned _activeFrame                       { 0 };

        vk::Extent2D _swapchainExtent               { 0 };
        vk::Format _swapchainFormat                 { vk::Format::eUndefined };

        vk::raii::Context _context;

        vk::raii::Instance _instance                { nullptr };
        vk::raii::SurfaceKHR _surface               { nullptr };
        vk::raii::DebugUtilsMessengerEXT _messenger { nullptr };
        vk::raii::PhysicalDevice _physicalDevice    { nullptr };
        vk::raii::Device _device                    { nullptr };
        vk::raii::SwapchainKHR _swapchain           { nullptr };
        vk::raii::RenderPass _renderPass            { nullptr };
        vk::raii::CommandPool _commandPool          { nullptr };
        vk::raii::Queue _graphicsQueue              { nullptr };
        unsigned _graphicsQueueIndex                { 0 };
        vk::raii::Queue _presentQueue               { nullptr };
        unsigned _presentQueueIndex                 { 0 };

        std::vector<vk::raii::ImageView> _imageViews;
        std::vector<vk::raii::Framebuffer> _framebuffers;

        std::vector<vk::raii::Semaphore> _imageReadySemaphores;
        std::vector<vk::raii::Semaphore> _renderCompleteSemaphores;
        std::vector<vk::raii::Fence> _inFlightFences;

        VulkanUtilities::DescriptorAllocator _descriptorAllocator;
        VulkanUtilities::VmaAllocator _vmaAllocator;
        VulkanUtilities::ImmediateSubmit _immediateSubmit;

        VulkanScene _scene;
        VulkanInitilization _initilization;
    };
}
