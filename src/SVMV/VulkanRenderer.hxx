#pragma once

#include <vulkan/vulkan.hpp>
#include <VkBootstrap.h>
#include <shaderc/shaderc.hpp>

#include <GLFW/glfw3.h>

#include <SVMV/Scene.hxx>
#include <SVMV/VulkanVertex.hxx>
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
        bool resized;

    private:
        VulkanScene _scene;

        unsigned _framesInFlight;
        unsigned _activeFrame;

        // seperate fields for bootstrap objects because they only contain c-style vulkan handles
        vkb::Instance _bootstrapInstance;
        vkb::Device _bootstrapDevice;
        vkb::Swapchain _bootstrapSwapchain;

        vk::Instance _instance;
        vk::DebugUtilsMessengerEXT _messenger;
        vk::PhysicalDevice _physicalDevice;
        vk::Device _device;
        vk::SwapchainKHR _swapchain;

        vk::SurfaceKHR _surface;

        vk::Queue _graphicsQueue;
        unsigned _graphicsQueueFamily;
        vk::Queue _presentQueue;
        unsigned _presentQueueFamily;

        std::vector<vk::Image> _images;
        std::vector<vk::ImageView> _imageViews;
        std::vector<vk::Framebuffer> _framebuffers;

        vk::RenderPass _renderPass;

        vk::CommandPool _commandPool;

        std::vector<vk::Semaphore> _imageReadySemaphores;
        std::vector<vk::Semaphore> _renderCompleteSemaphores;
        std::vector<vk::Fence> _inFlightFences;

    public:
        VulkanRenderer();
        VulkanRenderer(unsigned framesInFlight);

        void createInstance();
        void initializeRenderer(vk::SurfaceKHR surface);
        void loadScene(std::shared_ptr<Scene> scene);

        void draw();

        void cleanup();

        vk::Instance getInstance();
        vk::Device getDevice();

    private:
        void setSurface(vk::SurfaceKHR surface);

        void createDevice();
        void getQueues();
        void createSwapchain();
        void recreateSwapchain();

        void createRenderPass();
        void createFramebuffers();
        void createCommandPool();
        void createSynchronisationObjects();
    };
}
