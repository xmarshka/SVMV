#pragma once

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
        UNDEFINED, POSITION, NORMAL, TANGENT, TEXCOORD_0, COLOR_0
    };

    struct Attribute
    {
        Attribute() = default;

        Attribute(const Attribute&) = delete;
        Attribute& operator=(const Attribute&) = delete;

        Attribute(Attribute&& other) noexcept
        {
            this->attributeType = other.attributeType;
            this->elements = std::move(other.elements);
            this->type = other.type;
            this->size = other.size;
            this->count = other.count;
            this->componentCount = other.componentCount;

            other.attributeType = AttributeType::UNDEFINED;
            other.type = Type::UNDEFINED;
            other.size = 0;
            other.count = 0;
            other.componentCount = 0;
        }

        Attribute& operator=(Attribute&& other) noexcept
        {
            if (this != &other)
            {
                this->attributeType = other.attributeType;
                this->elements = std::move(other.elements);
                this->type = other.type;
                this->size = other.size;
                this->count = other.count;
                this->componentCount = other.componentCount;

                other.attributeType = AttributeType::UNDEFINED;
                other.type = Type::UNDEFINED;
                other.size = 0;
                other.count = 0;
                other.componentCount = 0;
            }

            return *this;
        }

        ~Attribute() = default;

        AttributeType attributeType{ AttributeType::UNDEFINED };
        std::unique_ptr<std::byte[]> elements{nullptr};

        Type type{ Type::UNDEFINED };
        size_t size{ 0 };
        size_t count{ 0 };
        int componentCount{ 0 };
    };
}