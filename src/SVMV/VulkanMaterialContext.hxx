#pragma once

#include <SVMV/VulkanDrawable.hxx>
#include <SVMV/VulkanMaterial.hxx>

#include <vector>

namespace SVMV
{
    struct VulkanDrawable;

    struct VulkanMaterialContext
    {
        std::vector<VulkanDrawable> drawables;
        const vk::raii::Pipeline* pipeline{ nullptr };
        const vk::raii::PipelineLayout* pipelineLayout{ nullptr };
    };
}
