#pragma once

#include <SVMV/Material.hxx>
#include <SVMV/Property.hxx>
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

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
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
            glm::vec4 baseColorFactor                   { 1.0f };
            glm::vec4 roughnessMetallicNormalFactors    { 1.0f }; // same format as the roughnessMetallicTexture (G and B channels), with the R channel containing a bool for the presence of the normal texture
            glm::vec4 emissiveFactor                    { 1.0f };

            // padding to align to 256 bytes
            glm::vec4 padding[13];
        };

        struct MaterialResources
        {
            VulkanUniformBuffer uniformBuffer;

            VulkanImage baseColorImage;
            VulkanImage normalImage;
            VulkanImage metallicRoughnessImage;
            VulkanImage occlusionImage;
            VulkanImage emissiveImage;

            vk::raii::Sampler baseColorSampler              { nullptr };
            vk::raii::Sampler normalSampler                 { nullptr };
            vk::raii::Sampler metallicRoughnessSampler      { nullptr };
            vk::raii::Sampler occlusionSampler              { nullptr };
            vk::raii::Sampler emissiveSampler               { nullptr };
        };

    public:
        GLTFPBRMaterial() = default;
        GLTFPBRMaterial(
            vk::raii::Device* device, VmaAllocator memoryAllocator, VulkanUtilities::ImmediateSubmit* immediateSubmit, const vk::raii::RenderPass& renderPass, const vk::raii::DescriptorSetLayout& globalDescriptorSetLayout,
            const vk::raii::DescriptorSetLayout& lightDescriptorSetLayout, VulkanUtilities::DescriptorAllocator* descriptorAllocator, VulkanDescriptorWriter* descriptorWriter, const shaderc::Compiler& compiler
        );

        GLTFPBRMaterial(const GLTFPBRMaterial&) = delete;
        GLTFPBRMaterial& operator=(const GLTFPBRMaterial&) = delete;

        GLTFPBRMaterial(GLTFPBRMaterial&& other) noexcept;
        GLTFPBRMaterial& operator=(GLTFPBRMaterial&& other) noexcept;

        ~GLTFPBRMaterial() = default;

        vk::DescriptorSet createDescriptorSet(std::shared_ptr<Material> material);

        const vk::raii::Pipeline* getPipeline() const;
        const vk::raii::PipelineLayout* getPipelineLayout() const;

    private:
        void createDefaultResources();

        void processUniformParameters(vk::raii::DescriptorSet& descriptorSet, MaterialResources& resources, std::shared_ptr<Material> material);
        void processTextures(vk::raii::DescriptorSet& descriptorSet, MaterialResources& resources, std::shared_ptr<Material> material);

        // TODO: change the TextureProperty to be a reference instead
        void processCombinedImageSampler(vk::raii::DescriptorSet& descriptorSet, int binding, VulkanImage& image, vk::raii::Sampler& sampler, const TextureProperty* textureProperty, vk::Format imageFormat);

    private:
        vk::raii::Device* _device                                       { nullptr };
        VmaAllocator _memoryAllocator                                   { nullptr };
        VulkanUtilities::ImmediateSubmit* _immediateSubmit              { nullptr };
        VulkanUtilities::DescriptorAllocator* _descriptorAllocator      { nullptr };
        VulkanDescriptorWriter* _descriptorWriter                       { nullptr };

        vk::raii::Pipeline _pipeline                            { nullptr };
        vk::raii::PipelineLayout _pipelineLayout                { nullptr };
        vk::raii::DescriptorSetLayout _descriptorSetLayout      { nullptr };

        VulkanShader _vertexShader;
        VulkanShader _fragmentShader;

        vk::raii::Sampler _defaultSampler       { nullptr };
        VulkanImage _defaultBaseColorImage;
        VulkanImage _defaultNormalImage;
        VulkanImage _defaultMetallicRoughnessImage;
        VulkanImage _defaultOcclusionImage;
        VulkanImage _defaultEmissiveImage;

        std::vector<MaterialResources> _resources;
        std::vector<vk::raii::DescriptorSet> _descriptorSets;
    };
}