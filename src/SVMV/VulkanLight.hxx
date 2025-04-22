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
        struct PointLight
        {
            glm::vec3 position;
            glm::vec3 flux;

            vk::raii::DescriptorSet descriptorSet{ nullptr };
        };

    public:
        VulkanLight() = default;

        VulkanLight(
            glm::vec3 position, glm::vec3 flux, vk::raii::Device* device, VmaAllocator memoryAllocator, const vk::raii::DescriptorSetLayout& lightDescriptorSetLayout,
            VulkanUtilities::DescriptorAllocator* descriptorAllocator, VulkanDescriptorWriter* descriptorWriter, int framesInFlight
            );

        const vk::raii::DescriptorSet* getDescriptorSet(int frameIndex) const;

        void setLightData(glm::vec3 position, glm::vec3 flux);
        void updateLightDescriptor(int frameIndex);
        
    private:
        std::vector<vk::raii::DescriptorSet> _descriptorSets;
        VulkanUniformBuffer _uniformBuffer;

        VulkanDescriptorWriter* _descriptorWriter{ nullptr };
    };
}