#pragma once

#include <glm/glm.hpp>

#include <cstdint>
#include <memory>

namespace SVMV
{
    struct MaterialInstance;

    struct VulkanDrawable
    {
        uint32_t firstIndex;
        uint32_t indexCount;

        uint32_t vertexCount;

        glm::mat4 modelMatrix;

        std::shared_ptr<MaterialInstance> materialInstance;

        VulkanDrawable() : modelMatrix(1.0f), firstIndex(0), indexCount(0), vertexCount(0) {}
        VulkanDrawable(glm::mat4 matrix) : modelMatrix(matrix), firstIndex(0), indexCount(0), vertexCount(0) {}
    };
}
