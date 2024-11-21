#pragma once

#include <vulkan/vulkan.hpp>
#include <VkBootstrap.h>
#include <vk_mem_alloc.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <SVMV/Scene.hxx>
#include <SVMV/Node.hxx>
#include <SVMV/Mesh.hxx>
#include <SVMV/Primitive.hxx>
#include <SVMV/Attribute.hxx>
#include <SVMV/VulkanDrawableCollection.hxx>
#include <SVMV/VulkanDrawable.hxx>
#include <SVMV/VulkanShader.hxx>
#include <SVMV/VulkanShaderStructures.hxx>
#include <SVMV/VulkanVertex.hxx>
#include <SVMV/VulkanBuffer.hxx>
#include <SVMV/VulkanMaterial.hxx>

#include <vector>
#include <array>
#include <unordered_map>

namespace SVMV
{
    class VulkanScene
    {
    private:
        vk::Device _device;
        vk::RenderPass _renderPass;

        vkb::Swapchain _swapchain;

        vk::CommandPool _commandPool;
        vk::Queue _queue;
        VmaAllocator _allocator;

        std::unordered_map<MaterialName, VulkanDrawableCollection> _collectionMap;

    public:
        VulkanScene();
        VulkanScene(vk::PhysicalDevice physicalDevice, vk::Instance instance, vk::Device device, vk::CommandPool commandPool, vk::Queue queue);

        ~VulkanScene();

        void setScene(std::shared_ptr<Scene> scene, vk::RenderPass renderPass, vkb::Swapchain swapchain, unsigned int framesInFlight);
        std::vector<vk::CommandBuffer> recordFrameCommandBuffers(unsigned int frame, vk::Framebuffer framebuffer, const unsigned int viewportWidth, const unsigned int viewportHeight);
    
    private:
        void divideSceneIntoCategories(std::shared_ptr<Scene> scene, std::shared_ptr<Node> rootNode);
        void loadSceneToGPUMemory(std::shared_ptr<Scene> scene, vkb::Swapchain swapchain, unsigned int framesInFlight);
        void loadCollectionVerticesToBuffer(VulkanDrawableCollection& collection, VulkanBuffer& stagingBuffer);

        void createCollectionCommandBuffers(unsigned int framesInFlight);

        unsigned char getVertexAttributeCategory(const std::shared_ptr<Primitive> primitive);
    };
}
