#pragma once

#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include <vector>
#include <memory>
#include <algorithm>

namespace SVMV
{
    class Material;

    class Primitive
    {
    public:
        std::vector<glm::aligned_vec3> positions;
        std::vector<glm::aligned_vec3> normals;
        std::vector<glm::vec4> tangents;
        std::vector<glm::vec2> texcoords_0;
        std::vector<glm::vec4> colors_0;

        std::vector<uint32_t> indices;
        std::shared_ptr<Material> material;
    };
}