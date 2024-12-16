#pragma once

#include <vulkan/vulkan.hpp>
#include <memory>

namespace SVMV
{
    enum class MaterialName
    {
        GLTFPBR, Other
    };

    struct MaterialPipeline
    {
        vk::raii::Pipeline pipeline;
        vk::raii::PipelineLayout layout;
    };

    struct MaterialInstance
    {
        vk::raii::DescriptorSet descriptorSet;

        MaterialName materialName;
    };
}
