#pragma once

#include <SVMV/Material.hxx>
#include <SVMV/Texture.hxx>
#include <SVMV/VulkanMaterial.hxx>
#include <SVMV/VulkanImage.hxx>
#include <SVMV/VulkanBuffer.hxx>
#include <SVMV/VulkanShader.hxx>
#include <SVMV/VulkanShaderStructures.hxx>
#include <SVMV/VulkanUtilities.hxx>
#include <SVMV/VulkanDescriptorWriter.hxx>

#include <vulkan/vulkan.hpp>
#include <VkBootstrap.h>

#include <glm/glm.hpp>

#include <vector>
#include <memory>

namespace SVMV
{
    class GLTFPBRMaterial
    {
    public:
        struct MaterialUniformParameters
        {
            glm::vec4 baseColorFactor           { 1.0f };
            glm::vec4 roughnessMetallicFactors  { 1.0f };

            // padding to align to 256 bytes
            glm::vec4 padding[14];
        };

        struct MaterialResources
        {
            VulkanImage colorImage;
            vk::raii::Sampler colorSampler;
            VulkanBuffer buffer;
        };

    public:
        GLTFPBRMaterial() = default;
        GLTFPBRMaterial(
            vk::raii::Device* device, VmaAllocator memoryAllocator, const vk::raii::RenderPass& renderPass, const vk::raii::DescriptorSetLayout& globalDescriptorSetLayout,
            VulkanUtilities::DescriptorAllocator* descriptorAllocator, VulkanDescriptorWriter* descriptorWriter, const shaderc::Compiler& compiler
        );

        GLTFPBRMaterial(const GLTFPBRMaterial&) = delete;
        GLTFPBRMaterial& operator=(const GLTFPBRMaterial&) = delete;

        GLTFPBRMaterial(GLTFPBRMaterial&& other) noexcept;
        GLTFPBRMaterial& operator=(GLTFPBRMaterial&& other) noexcept;

        ~GLTFPBRMaterial() = default;

        std::shared_ptr<vk::raii::DescriptorSet> createDescriptorSet(std::shared_ptr<Material> material);

        const vk::raii::Pipeline* getPipeline() const;
        const vk::raii::PipelineLayout* getPipelineLayout() const;

    private:
        vk::raii::Device* _device{ nullptr };
        VmaAllocator _memoryAllocator{ nullptr };
        VulkanUtilities::ImmediateSubmit* _immediateSubmit{ nullptr };
        VulkanUtilities::DescriptorAllocator* _descriptorAllocator{ nullptr };
        VulkanDescriptorWriter* _descriptorWriter{ nullptr };

        vk::raii::Pipeline _pipeline{ nullptr };
        vk::raii::PipelineLayout _pipelineLayout{ nullptr };
        vk::raii::DescriptorSetLayout _descriptorSetLayout{ nullptr };

        VulkanShader _vertexShader;
        VulkanShader _fragmentShader;

        std::vector<MaterialResources> _resources;
    };
}