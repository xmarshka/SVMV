#pragma once

#include <memory>
#include <vector>

namespace SVMV
{
    class Primitive;

    class Mesh
    {
    public:
        std::vector<std::shared_ptr<Primitive>> primitives;
    };
}