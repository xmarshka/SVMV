#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <string>
#include <memory>
#include <unordered_map>

namespace SVMV
{
    struct Property;

    struct Material
    {
        std::string materialTypeName; // name of the material type (e.g. glTFPBR, UNLIT, etc.)
        std::string materialName; // specific name of the material (e.g. gold, oak, red_metal, etc.)

        std::unordered_map<std::string, std::shared_ptr<Property>> properties;
    };
}