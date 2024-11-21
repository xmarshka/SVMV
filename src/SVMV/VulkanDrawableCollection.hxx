#pragma once

#include <vulkan/vulkan.hpp>

#include <SVMV/VulkanBuffer.hxx>
#include <SVMV/VulkanMaterial.hxx>

#include <vector>
#include <memory>
#include <unordered_map>

namespace SVMV
{
    class Primitive;

    struct VulkanDrawable;
    class VulkanMaterial;

    struct VulkanDrawableCollection
    {
        std::vector<std::shared_ptr<Primitive>> sources;
        std::vector<VulkanDrawable> drawables;

        size_t totalVertexCount = 0;
        size_t totalIndexCount = 0;
        size_t totalIndexSize = 0;

        VulkanBuffer indexBuffer;
        VulkanBuffer vertexBuffer;

        MaterialPipeline materialPipeline;

        std::vector<vk::CommandBuffer> commandBuffers;
    };
}
