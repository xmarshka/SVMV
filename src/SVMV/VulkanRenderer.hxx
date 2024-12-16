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
        [[nodiscard]] GLFWwindow* getWindow() const noexcept;

    private:
        void preprocessScene(std::shared_ptr<Scene> scene); // TODO: generate material contexts and materials that appear in the scene, get attribute sizes and create the buffers in VulkanScene
        void generateDrawablesFromScene(std::shared_ptr<Node> node, glm::mat4 baseTransform); // TODO: generate drawables and load mesh data to staging buffers, recursive
        void copyStagingBuffersToGPUBuffers(); // TODO: in the name

        void recreateSwapchain();
        void createRenderPass();

        static void resized(GLFWwindow* window, int width, int height);
        static void minimized(GLFWwindow* window, int minimized);

    private:
        VulkanUtilities::GLFWwindowWrapper _window;
        shaderc::Compiler _shaderCompiler;

        bool _resized                               { false };
        int _framesInFlight                         { 0 };
        int _activeFrame                            { 0 };

        vk::Extent2D _swapchainExtent               { 0 };
        vk::Format _swapchainFormat                 { vk::Format::eUndefined };

        vk::raii::Context _context;

        vk::raii::Instance _instance                { nullptr };
        vk::raii::DebugUtilsMessengerEXT _messenger { nullptr };
        vk::raii::SurfaceKHR _surface               { nullptr };
        vk::raii::PhysicalDevice _physicalDevice    { nullptr };
        vk::raii::Device _device                    { nullptr };
        vk::raii::SwapchainKHR _swapchain           { nullptr };
        vk::raii::RenderPass _renderPass            { nullptr };
        vk::raii::CommandPool _commandPool          { nullptr };
        vk::raii::Queue _graphicsQueue              { nullptr };
        int _graphicsQueueIndex                     { 0 };
        vk::raii::Queue _presentQueue               { nullptr };
        int _presentQueueIndex                      { 0 };

        std::vector<vk::raii::ImageView> _imageViews;
        std::vector<vk::raii::Framebuffer> _framebuffers;

        std::vector<vk::raii::Semaphore> _imageReadySemaphores;
        std::vector<vk::raii::Semaphore> _renderCompleteSemaphores;
        std::vector<vk::raii::Fence> _inFlightFences;

        VulkanUtilities::DescriptorAllocator _descriptorAllocator;
        VulkanUtilities::VmaAllocatorWrapper _vmaAllocator;
        VulkanUtilities::ImmediateSubmit _immediateSubmit;

        VulkanScene _scene;
        VulkanInitilization _initilization;

        std::unordered_map<std::shared_ptr<Primitive>, VulkanDrawable> _primitiveDrawableMap;
    };
}
