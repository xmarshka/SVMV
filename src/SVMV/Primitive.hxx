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
        std::set<Attribute> attributes;

        std::shared_ptr<Material> material;
    };
}