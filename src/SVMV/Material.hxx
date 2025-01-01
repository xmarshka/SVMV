#pragma once

#include <glm/glm.hpp>

#include <string>
#include <memory>
#include <unordered_map>

namespace SVMV
{
    struct Property;

    struct Material
    {
        std::string materialTypeName;
        std::string materialName;

        std::unordered_map<std::string, Property> properties;
    };
}