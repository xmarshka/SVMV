#pragma once

#include <memory>

namespace SVMV
{
    struct Texture
    {
        unsigned width      { 0 };
        unsigned height     { 0 };

        std::unique_ptr<std::byte[]> data; // in RGBA format
        size_t size     { 0 };
    };
}
