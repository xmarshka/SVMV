#pragma once

#include <memory>
#include <vector>

namespace SVMV
{
    struct Node;
    struct Mesh;
    struct Material;

    struct Scene
    {
        std::shared_ptr<Node> root;

        std::vector<std::shared_ptr<Mesh>> meshes;
        std::vector<std::shared_ptr<Material>> materials;
    };
}
