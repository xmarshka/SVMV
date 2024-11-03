#pragma once

#include <glm/glm.hpp>

#include <cstdint>

namespace SVMV
{
    struct VulkanDrawable
    {
        uint32_t firstIndex;
        uint32_t indexCount;

        uint32_t vertexCount;
    };
}
