#pragma once

#include <SVMV/Attribute.hxx>

#include <vector>
#include <memory>
#include <algorithm>

namespace SVMV
{
    class Material;

    class Primitive
    {
    public:
        std::vector<std::shared_ptr<Attribute>> attributes;
        std::vector<uint32_t> indices;
        std::shared_ptr<Material> material;

    public:
        std::shared_ptr<Attribute> getAttribute(Attribute::AttributeType type);
    };
}