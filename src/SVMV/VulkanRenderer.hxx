#pragma once

#include <vulkan/vulkan_raii.hpp>
#include <vk_mem_alloc.h>
#include <VkBootstrap.h>
#include <shaderc/shaderc.hpp>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

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
        VulkanRenderer(int width, int height, const std::string& name, unsigned framesInFlight, const GLFWwindowWrapper& window);

        VulkanRenderer(const VulkanRenderer&) = delete;
        VulkanRenderer& operator=(const VulkanRenderer&) = delete;

        VulkanRenderer(VulkanRenderer&&) = delete;
        VulkanRenderer& operator=(VulkanRenderer&&) = delete;

        ~VulkanRenderer() = default;

        void draw();

        void loadScene(std::shared_ptr<Scene> scene);

        void setCamera(glm::vec3 position, glm::vec3 lookDirection, glm::vec3 upDirection, float fieldOfView);

        void resize(int width, int height);
        void minimize();
        void maximize();

        [[nodiscard]] const vk::Device getDevice() const noexcept;

    private:
        void recordDrawCommands(int activeFrame, const vk::raii::Framebuffer& framebuffer);

        void preprocessScene(std::shared_ptr<Scene> scene);
        void generateDrawablesFromScene(std::shared_ptr<Node> node, glm::mat4 baseTransform);
        void copyStagingBuffersToGPUBuffers();

        void recreateSwapchain();
        void createRenderPass();
        void createShadowMappingRenderPass();
        void createShadowMappingPipeline();
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

        vk::raii::CommandBuffers _drawCommandBuffers{ nullptr };

        VulkanUtilities::DescriptorAllocator _descriptorAllocator;
        VulkanDescriptorWriter _descriptorWriter;
        VulkanUtilities::VmaAllocatorWrapper _vmaAllocator;
        VulkanUtilities::ImmediateSubmit _immediateSubmit;

        std::vector<vk::raii::ImageView> _imageViews;
        std::vector<vk::raii::Framebuffer> _framebuffers;
        
        VulkanImage _depthBuffer;
        VulkanImage _shadowBuffer;
        vk::raii::Framebuffer _shadowFramebuffer        { nullptr };

        std::vector<vk::raii::Semaphore> _imageReadySemaphores;
        std::vector<vk::raii::Semaphore> _renderCompleteSemaphores;
        std::vector<vk::raii::Fence> _inFlightFences;

        vk::raii::DescriptorSetLayout _globalDescriptorSetLayout    { nullptr };
        std::vector<VulkanUniformBuffer> _globalDescriptorSetBuffers;
        std::vector<vk::raii::DescriptorSet> _globalDescriptorSets;

        VulkanScene _scene;

        vk::raii::Pipeline _shadowMappingPipeline           { nullptr };
        vk::raii::RenderPass _shadowMappingRenderPass       { nullptr };

        VulkanInitilization _initilization;
    };
}
