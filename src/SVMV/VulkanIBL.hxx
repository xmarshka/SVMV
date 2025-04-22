#pragma once

#include <vulkan/vulkan_raii.hpp>

#include <stb/stb_image.h>

#include <SVMV/VulkanImage.hxx>

namespace SVMV
{
    class VulkanIBL
    {
    public:
        VulkanIBL(const std::string& textureFileName);

    private:
        VulkanImage _cubeMap;
        VulkanImage _irradianceMap;
        VulkanImage _preFilteredMap;
        VulkanImage _LUT;

        vk::raii::DescriptorSetLayout _descriptorSetLayout      { nullptr };
        vk::raii::DescriptorSet _descriptorSet                  { nullptr };
    };
}