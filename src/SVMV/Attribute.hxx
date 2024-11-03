#pragma once

#include <memory>
#include <vector>

namespace SVMV
{
    class Attribute
    {
    public:
        enum class AttributeType
        {
            POSITION, COLOR, TEXCOORD, NORMAL, TANGENT, BINORMAL
        };

    public:
        std::vector<uint8_t> data;
        size_t count;
        size_t components;
        size_t componentSize; // in bytes
        size_t stride; // in bytes

        AttributeType type;
    };
}
