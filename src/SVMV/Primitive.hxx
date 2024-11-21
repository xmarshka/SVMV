#pragma once

#include <glm/glm.hpp>

#include <vector>
#include <memory>
#include <algorithm>

namespace SVMV
{
    class Material;

    class Primitive
    {
    public:
        std::vector<glm::fvec3> positions;
        std::vector<glm::fvec3> normals;
        std::vector<glm::fvec4> tangens;
        std::vector<glm::fvec2> texcoords_0;
        std::vector<glm::fvec4> colors_0;

        std::vector<uint32_t> indices;
        std::shared_ptr<Material> material;
    };
}