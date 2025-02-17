#pragma once

#include <SVMV/Primitive.hxx>
#include <SVMV/Attribute.hxx>
#include <SVMV/VulkanDrawable.hxx>
#include <SVMV/VulkanGLTFPBRMaterial.hxx>
#include <SVMV/VulkanMaterialContext.hxx>

#include <vector>
#include <string>
#include <memory>

namespace SVMV
{
    struct VertexAttribute
    {
        AttributeType type{ AttributeType::UNDEFINED };

        VulkanGPUBuffer gpuBuffer;
        vk::DeviceAddress gpuBufferAddressCounter{ 0 };

        VulkanStagingBuffer stagingBuffer;
    };

    struct VulkanScene
    {
        VulkanGPUBuffer indexGPUBuffer;
        VulkanStagingBuffer indexStagingBuffer;
        int indexCounter{ 0 };

        VulkanGPUBuffer modelMatrixGPUBuffer;
        VulkanStagingBuffer modelMatrixStagingBuffer;

        VulkanGPUBuffer normalMatrixGPUBuffer;
        VulkanStagingBuffer normalMatrixStagingBuffer;
        int drawableCounter{ 0 };

        std::vector<VertexAttribute> attributes; // holds the buffers containing attribute data for all drawables in the scene

        std::unordered_map<std::string, VulkanMaterialContext> contexts;
        std::unordered_map<std::shared_ptr<Primitive>, VulkanDrawable> primitiveDrawableMap;

        GLTFPBRMaterial glTFPBRMaterial;
    };
}
