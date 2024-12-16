#pragma once

#include <vulkan/vulkan_raii.hpp>
#include <VkBootstrap.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <SVMV/Scene.hxx>
#include <SVMV/Node.hxx>
#include <SVMV/Mesh.hxx>
#include <SVMV/Primitive.hxx>
#include <SVMV/VulkanDrawable.hxx>
#include <SVMV/VulkanShader.hxx>
#include <SVMV/VulkanShaderStructures.hxx>
#include <SVMV/VulkanMaterial.hxx>
#include <SVMV/VulkanGLTFPBRMaterial.hxx>
#include <SVMV/VulkanUtilities.hxx>
#include <SVMV/VulkanMaterialContext.hxx>

#include <vector>
#include <array>
#include <unordered_map>
#include <memory>
#include <iostream>

namespace SVMV
{
    // TODO:    move functionality to VulkanRenderer, change the VulkanScene class to hold GPU memory data (attributes of all primitives and all materials and contexts with drawables)
    //          create a class to hold all the attribute buffers and accept primitive objects and copy the staging buffers to the GPU buffers
    //          restructure collections into contexts, have them hold drawables and pipelines
    //          command buffer recording must happen in VulkanRenderer, VulkanScene should just hold and process data
    //
    //          VulkanContext, VulkanVertexAttributes

    // TODO:    also... rewrite VulkanShader...

    struct VertexAttribute
    {
        VulkanGPUBuffer gpuBuffer;
        vk::DeviceAddress gpuBufferAddressCounter{ 0 };

        VulkanStagingBuffer stagingBuffer;
    };

    struct VulkanScene
    {
        std::vector<VulkanMaterialContext> contexts;

        VertexAttribute indices;
        int indexCounter{ 0 };

        VertexAttribute positions;
        VertexAttribute normals;
        VertexAttribute tangents;
        VertexAttribute texcoords_0;
        VertexAttribute colors_0;
    };

    //class VulkanScene
    //{
    //private:
    //    vk::raii::Device* _device;
    //    vk::raii::RenderPass* _renderPass;
    //    vk::raii::CommandPool* _commandPool;
    //    vk::raii::Queue* _queue;
    //    unsigned _queueIndex;

    //    VmaAllocator _allocator;
    //    VulkanDescriptorAllocator* _descriptorAllocator;
    //    VulkanUtilities::ImmediateSubmit* _immediateSubmit;

    //    std::unordered_map<MaterialName, std::unique_ptr<VulkanDrawableCollection>> _collectionMap;
    //    GLTFPBRMaterial _GLTFPBRMaterial;

    //public:
    //    VulkanScene();
    //    VulkanScene(vk::raii::PhysicalDevice* physicalDevice, vk::raii::Instance* instance, vk::raii::Device* device, vk::raii::CommandPool* commandPool, vk::raii::Queue* queue, unsigned queueIndex);
    //    ~VulkanScene();
    //    
    //    void initialize(vk::raii::PhysicalDevice* physicalDevice, vk::raii::Instance* instance, vk::raii::Device* device, vk::raii::CommandPool* commandPool, vk::raii::Queue* queue, unsigned queueIndex);
    //    void free();

    //    void setScene(std::shared_ptr<Scene> scene, vk::raii::RenderPass* renderPass, const vk::Extent2D& extent, unsigned int framesInFlight);
    //    std::vector<vk::CommandBuffer> recordFrameCommandBuffers(unsigned int frame, const vk::raii::Framebuffer& framebuffer, const unsigned int viewportWidth, const unsigned int viewportHeight);
    //
    //private:
    //    void createCollectionsFromScene(std::shared_ptr<Scene> scene);
    //    void createDrawablesFromScene(std::shared_ptr<Scene> scene, std::shared_ptr<Node> rootNode);
    //    void loadPrimitiveAttributeDataToBuffer(const std::unique_ptr<VulkanDrawableCollection>& collection, std::shared_ptr<Primitive> primitive);
    //    void copyStagingBufferDataToBuffers(const std::unique_ptr<VulkanDrawableCollection>& collection);

    //    void copyIndicesData(uint32_t* destination, const std::vector<uint32_t>& attributeVector, size_t& offset);

    //    // TODO: add 'requires' to templates and check the template values(?)
    //    template <typename type, typename attributeType, unsigned componentCount>
    //    void copyAttributeData(type* destination, const std::vector<glm::vec<componentCount, attributeType>>& attributeVector, size_t& offset);

    //    void createCollectionCommandBuffers(unsigned int framesInFlight);
    //};
}
