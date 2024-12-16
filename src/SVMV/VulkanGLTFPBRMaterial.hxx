#pragma once

#include <SVMV/Material.hxx>
#include <SVMV/Texture.hxx>
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
        vk::raii::Device* _device;
        VmaAllocator _memoryAllocator;
        VulkanUtilities::ImmediateSubmit* _immediateSubmit;
        VulkanUtilities::DescriptorAllocator* _descriptorAllocator;

        vk::raii::Pipeline _pipeline;
        vk::raii::PipelineLayout _pipelineLayout;
        vk::raii::DescriptorSetLayout _descriptorSetLayout;

        struct MaterialUniformParameters
        {
            glm::vec4 baseColorFactor;
            glm::vec4 roughnessMetallicFactors;
            glm::vec4 padding[14];
        };

        struct MaterialResources
        {
            std::shared_ptr<VulkanImage> colorImage;
            vk::raii::Sampler colorSampler;
            std::shared_ptr<VulkanBuffer> buffer;
        };

        std::vector<MaterialResources> _resources;
        std::vector<std::shared_ptr<MaterialInstance>> _instances;

    public:
        GLTFPBRMaterial();
        GLTFPBRMaterial(vk::raii::Device* device, VmaAllocator allocator, const vk::raii::RenderPass& renderPass, const vk::Extent2D& extent, VulkanUtilities::DescriptorAllocator* descriptorAllocator);
        ~GLTFPBRMaterial();

        void initialize(vk::raii::Device* device, VmaAllocator allocator, const vk::raii::RenderPass& renderPass, const vk::Extent2D& extent, VulkanUtilities::DescriptorAllocator* descriptorAllocator);
        void free();

        std::shared_ptr<MaterialInstance> generateMaterialInstance(std::shared_ptr<Material> material);
        MaterialPipeline getMaterialPipeline();

    private:
        void generatePipeline(const vk::raii::RenderPass& renderPass, const vk::Extent2D& extent);
    };
}