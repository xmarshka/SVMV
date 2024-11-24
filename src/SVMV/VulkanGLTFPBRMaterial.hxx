#pragma once

#include <SVMV/Material.hxx>
#include <SVMV/Texture.hxx>
#include <SVMV/VulkanDescriptorAllocator.hxx>
#include <SVMV/VulkanMaterial.hxx>
#include <SVMV/VulkanImage.hxx>
#include <SVMV/VulkanBuffer.hxx>
#include <SVMV/VulkanShader.hxx>
#include <SVMV/VulkanShaderStructures.hxx>
#include <SVMV/VulkanUtilities.hxx>

#include <vulkan/vulkan.hpp>
#include <VkBootstrap.h>

#include <glm/glm.hpp>

#include <vector>
#include <memory>

namespace SVMV
{
    class GLTFPBRMaterial
    {
    private:
        vk::Device _device;
        VmaAllocator _memoryAllocator;
        VulkanUtilities::ImmediateSubmit _immediateSubmit;

        vk::Pipeline _pipeline;
        vk::PipelineLayout _pipelineLayout;

        vk::DescriptorSetLayout _layout;

        struct MaterialUniformParameters
        {
            glm::vec4 baseColorFactor;
            glm::vec4 roughnessMetallicFactors;
            glm::vec4 padding[14];
        };

        struct MaterialResources
        {
            std::shared_ptr<VulkanImage> colorImage;
            vk::Sampler colorSampler;
            std::shared_ptr<VulkanBuffer> buffer;
        };

        std::vector<MaterialResources> _resources;
        std::vector<std::shared_ptr<MaterialInstance>> _instances;

    public:
        void initialize(vk::Device device, VmaAllocator allocator, vk::RenderPass renderPass, vkb::Swapchain swapchain);
        void free();

        std::shared_ptr<MaterialInstance> generateMaterialInstance(std::shared_ptr<Material> material, VulkanDescriptorAllocator& descriptorAllocator);
        MaterialPipeline getMaterialPipeline();

    private:
        void generatePipeline(vk::Device device, vk::RenderPass renderPass, vkb::Swapchain swapchain);
    };
}