#pragma once

#include <span>
#include <memory>
#include <cstddef>

namespace SVMV
{
    enum class Type : int
    {
        UNDEFINED, FLOAT, DOUBLE, UINT16, UINT32, INT16, INT32
    };

    enum class AttributeType : int
    {
        UNDEFINED, INDEX, POSITION, NORMAL, TANGENT, TEXCOORD_0, COLOR_0
    };

    struct Attribute
    {
        AttributeType attributeType{ AttributeType::UNDEFINED };
        std::unique_ptr<std::byte[]> elements{nullptr};

        Type type{ Type::UNDEFINED };
        size_t size{ 0 };
        size_t count{ 0 };
        int componentCount{ 0 };
    };
}