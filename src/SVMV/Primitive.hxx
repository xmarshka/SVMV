#pragma once

#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include <vector>
#include <set>
#include <memory>
#include <algorithm>

namespace SVMV
{
    struct Attribute;
    struct Material;

    struct Primitive
    {
        std::vector<uint32_t> indices;
        std::vector<Attribute> attributes;

        std::shared_ptr<Material> material;
    };
}