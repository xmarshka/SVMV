#pragma once

#include <memory>
#include <vector>

namespace SVMV
{
    class Node;
    class Mesh;
    class Material;

    class Scene
    {
    public:

        std::shared_ptr<Node> root;

        std::vector<std::shared_ptr<Mesh>> meshes;
        std::vector<std::shared_ptr<Material>> materials;
    };
}
