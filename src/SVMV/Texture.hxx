#pragma once

#include <span>

namespace SVMV
{
    struct Texture
    {
        unsigned width;
        unsigned height;

        std::span<uint8_t> data; // in RGBA format
    };
}
