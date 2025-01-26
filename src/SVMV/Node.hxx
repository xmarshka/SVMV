#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <memory>
#include <vector>

namespace SVMV
{
    struct Mesh;

    struct Node
    {
        glm::mat4 transform{ 1.0 }; // initialized to identity
        std::vector<std::shared_ptr<Node>> children;

        std::shared_ptr<Mesh> mesh;
    };
}