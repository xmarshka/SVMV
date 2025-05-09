#pragma once

#include <vulkan/vulkan_raii.hpp>
#include <glm/glm.hpp>

#include <SVMV/VulkanShaderStructures.hxx>
#include <SVMV/VulkanUtilities.hxx>
#include <SVMV/VulkanDescriptorWriter.hxx>

#include <vector>

namespace SVMV
{
    class VulkanLight
    {
    public:
        struct LightData
        {
            glm::vec4 position_0    { 0.0f };
            glm::vec4 flux_0        { 0.0f };

            glm::vec4 position_1    { 0.0f };
            glm::vec4 flux_1        { 0.0f };

            glm::vec4 position_2    { 0.0f };
            glm::vec4 flux_2        { 0.0f };

            glm::vec4 ambient       { 0.0f };
        };

    public:
        VulkanLight() = default;

        VulkanLight(
            glm::vec4 position, glm::vec4 flux, vk::raii::Device* device, VmaAllocator memoryAllocator, const vk::raii::DescriptorSetLayout& lightDescriptorSetLayout,
            VulkanUtilities::DescriptorAllocator* descriptorAllocator, VulkanDescriptorWriter* descriptorWriter, int framesInFlight
            );

        const vk::raii::DescriptorSet* getDescriptorSet(int frameIndex) const;

        void updateLightDescriptor(int frameIndex);
        
    public:
        LightData lightData;

    private:
        std::vector<vk::raii::DescriptorSet> _descriptorSets;
        VulkanUniformBuffer _uniformBuffer;

        VulkanDescriptorWriter* _descriptorWriter{ nullptr };
    };
}