#pragma once

#include <vulkan/vulkan_raii.hpp>
#include <vk_mem_alloc.h>
#include <VkBootstrap.h>
#include <shaderc/shaderc.hpp>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_vulkan.h>

#include <ImGuiFileDialog/ImGuiFileDialog.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <SVMV/Loader.hxx>
#include <SVMV/GLFWwindowWrapper.hxx>
#include <SVMV/Scene.hxx>
#include <SVMV/Node.hxx>
#include <SVMV/Mesh.hxx>
#include <SVMV/Primitive.hxx>
#include <SVMV/Attribute.hxx>
#include <SVMV/VulkanInitialization.hxx>
#include <SVMV/VulkanScene.hxx>
#include <SVMV/VulkanBuffer.hxx>
#include <SVMV/VulkanShaderStructures.hxx>
#include <SVMV/VulkanDescriptorWriter.hxx>
#include <SVMV/VulkanLight.hxx>

#include <memory>
#include <vector>
#include <fstream>
#include <iostream>
#include <sstream>
#include <cmath>

namespace SVMV
{
    class VulkanRenderer
    {
    public:
        VulkanRenderer() = default;
        VulkanRenderer(int width, int height, const std::string& name, unsigned framesInFlight, const GLFWwindowWrapper& window);

        VulkanRenderer(const VulkanRenderer&) = delete;
        VulkanRenderer& operator=(const VulkanRenderer&) = delete;

        VulkanRenderer(VulkanRenderer&&) = delete;
        VulkanRenderer& operator=(VulkanRenderer&&) = delete;

        ~VulkanRenderer();

        void draw();

        void loadScene(std::shared_ptr<Scene> scene);

        void setCamera(glm::vec3 position, glm::vec3 lookDirection, glm::vec3 upDirection, float fieldOfView);

        [[nodiscard]] const vk::Device getDevice() const noexcept;

    private:
        void recordDrawCommands(int activeFrame, const vk::raii::Framebuffer& framebuffer, const vk::raii::Framebuffer& imguiFramebuffer);

        void preprocessScene(std::shared_ptr<Scene> scene);
        void generateDrawablesFromScene(std::shared_ptr<Node> node, glm::mat4 baseTransform);
        void copyStagingBuffersToGPUBuffers();

        void recreateSwapchain();
        void createRenderPass();
        void createGlobalDescriptorSets();
        void createDepthBuffer();

    private:
        shaderc::Compiler _shaderCompiler;

        bool _resized           { false };
        int _framesInFlight     { 0 };
        int _activeFrame        { 0 };

        glm::mat4 _projectionMatrix     { 1.0f };
        glm::mat4 _viewMatrix           { 1.0f };
        glm::vec3 _cameraPosition       { 0.0f };

        vk::Extent2D _swapchainExtent   { 0 };
        vk::Format _swapchainFormat     { vk::Format::eUndefined };

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
        vk::raii::Queue _computeQueue               { nullptr };
        int _computeQueueIndex                      { 0 };

        vk::raii::CommandBuffers _drawCommandBuffers    { nullptr };

        vk::raii::RenderPass _imguiRenderPass               { nullptr };
        vk::raii::CommandPool _imguiCommandPool             { nullptr }; // unused?
        vk::raii::CommandBuffers _imguiCommandBuffers       { nullptr }; // unused?
        vk::raii::DescriptorPool _imguiDescriptorPool       { nullptr };
        std::vector<vk::raii::Framebuffer> _imguiFramebuffers;

        VulkanUtilities::DescriptorAllocator _descriptorAllocator;
        VulkanDescriptorWriter _descriptorWriter;
        VulkanUtilities::VmaAllocatorWrapper _vmaAllocator;
        VulkanUtilities::ImmediateSubmit _immediateSubmit;

        std::vector<vk::raii::ImageView> _imageViews;
        std::vector<vk::raii::Framebuffer> _framebuffers;
        
        VulkanImage _depthBuffer;

        std::vector<vk::raii::Semaphore> _imageReadySemaphores;
        std::vector<vk::raii::Semaphore> _renderCompleteSemaphores;
        std::vector<vk::raii::Fence> _inFlightFences;

        vk::raii::DescriptorSetLayout _globalDescriptorSetLayout        { nullptr };
        std::vector<VulkanUniformBuffer> _globalDescriptorSetBuffers;
        std::vector<vk::raii::DescriptorSet> _globalDescriptorSets;

        vk::raii::DescriptorSetLayout _lightDescriptorSetLayout         { nullptr };

        VulkanScene _scene;
        std::string _requestedScenePath;

        VulkanLight _light;

        VulkanInitilization _initilization;
    };
}
