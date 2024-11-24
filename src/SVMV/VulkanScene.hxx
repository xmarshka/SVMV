#pragma once

#include <vulkan/vulkan.hpp>
#include <VkBootstrap.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <SVMV/Scene.hxx>
#include <SVMV/Node.hxx>
#include <SVMV/Mesh.hxx>
#include <SVMV/Primitive.hxx>
#include <SVMV/VulkanDrawableCollection.hxx>
#include <SVMV/VulkanDrawable.hxx>
#include <SVMV/VulkanShader.hxx>
#include <SVMV/VulkanShaderStructures.hxx>
#include <SVMV/VulkanVertex.hxx>
#include <SVMV/VulkanMaterial.hxx>
#include <SVMV/VulkanGLTFPBRMaterial.hxx>
#include <SVMV/VulkanDescriptorAllocator.hxx>
#include <SVMV/VulkanUtilities.hxx>

#include <vector>
#include <array>
#include <unordered_map>
#include <iostream>

namespace SVMV
{
    class VulkanScene
    {
    private:
        vk::Device _device;
        vk::RenderPass _renderPass;
        vk::CommandPool _commandPool;
        vk::Queue _queue;
        unsigned _queueIndex;

        vkb::Swapchain _swapchain;

        VmaAllocator _allocator;
        VulkanDescriptorAllocator _descriptorAllocator;
        VulkanUtilities::ImmediateSubmit _immediateSubmit;

        std::unordered_map<MaterialName, VulkanDrawableCollection> _collectionMap;
        GLTFPBRMaterial _GLTFPBRMaterial;

    public:
        VulkanScene();
        
        void initialize(vk::PhysicalDevice physicalDevice, vk::Instance instance, vk::Device device, vk::CommandPool commandPool, vk::Queue queue, unsigned queueIndex);
        void free();

        void setScene(std::shared_ptr<Scene> scene, vk::RenderPass renderPass, vkb::Swapchain swapchain, unsigned int framesInFlight);
        std::vector<vk::CommandBuffer> recordFrameCommandBuffers(unsigned int frame, vk::Framebuffer framebuffer, const unsigned int viewportWidth, const unsigned int viewportHeight);
    
    private:
        void createCollectionsFromScene(std::shared_ptr<Scene> scene);
        void createDrawablesFromScene(std::shared_ptr<Scene> scene, std::shared_ptr<Node> rootNode);
        void loadPrimitiveAttributeDataToBuffer(VulkanDrawableCollection& collection, std::shared_ptr<Primitive> primitive);
        void copyStagingBufferDataToBuffers(VulkanDrawableCollection& collection);

        void copyIndicesData(uint32_t* destination, const std::vector<uint32_t>& attributeVector, size_t& offset);

        template <typename type, typename attributeType, unsigned componentCount>
        void copyAttributeData(type* destination, const std::vector<glm::vec<componentCount, attributeType>>& attributeVector, size_t& offset);

        void createCollectionCommandBuffers(unsigned int framesInFlight);
    };
}
