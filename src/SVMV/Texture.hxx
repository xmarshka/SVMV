#pragma once

#include <span>

namespace SVMV
{
    class Texture
    {
    public:
        unsigned width;
        unsigned height;

        std::span<uint8_t> data; // in RGBA format
    };
}
