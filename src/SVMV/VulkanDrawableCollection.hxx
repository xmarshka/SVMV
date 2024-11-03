#pragma once

#include <vulkan/vulkan.hpp>

#include <SVMV/VulkanBuffer.hxx>

#include <vector>

namespace SVMV
{
    class Primitive;

    class VulkanDrawable;
    class VulkanMaterial;

    struct VulkanDrawableCollection
    {
        std::vector<std::shared_ptr<Primitive>> sources;

        size_t totalVertexCount;
        size_t totalIndexSize;

        std::vector<VulkanDrawable> objects;

        VulkanBuffer indices;
        VulkanBuffer positions;
        VulkanBuffer colors;
        VulkanBuffer normals;

        std::shared_ptr<VulkanMaterial> material;

        vk::PipelineLayout layout;
        vk::Pipeline graphicsPipeline;

        std::vector<vk::CommandBuffer> commandBuffers;
    };
}
