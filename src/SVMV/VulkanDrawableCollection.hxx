#pragma once

#include <vulkan/vulkan.hpp>

#include <SVMV/VulkanBuffer.hxx>

#include <vector>
#include <memory>
#include <unordered_map>

namespace SVMV
{
    class Primitive;

    class VulkanDrawable;
    class VulkanMaterial;

    struct VulkanDrawableCollection
    {
        std::vector<std::shared_ptr<Primitive>> sources;
        std::vector<VulkanDrawable> drawables;

        size_t totalVertexCount = 0;
        size_t totalIndexCount = 0;
        size_t totalIndexSize = 0;

        VulkanBuffer indices;
        
        std::vector<VulkanBuffer> attributeBuffers;

        std::shared_ptr<VulkanMaterial> material;

        vk::PipelineLayout layout;
        vk::Pipeline graphicsPipeline;

        std::vector<vk::CommandBuffer> commandBuffers;
    };
}
