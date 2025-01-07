#pragma once

#include <memory>
#include <vector>

namespace SVMV
{
    struct Primitive;

    struct Mesh
    {
        std::vector<std::shared_ptr<Primitive>> primitives;
    };
}