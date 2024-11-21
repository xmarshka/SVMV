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
        vk::Pipeline pipeline;
        vk::PipelineLayout layout;
    };

    struct MaterialInstance
    {
        vk::DescriptorSet descriptorSet;

        MaterialName materialName;
    };
}
