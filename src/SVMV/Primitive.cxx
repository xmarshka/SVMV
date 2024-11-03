#include <SVMV/Primitive.hxx>

using namespace SVMV;

std::shared_ptr<Attribute> Primitive::getAttribute(Attribute::AttributeType type)
{
    auto iterator = std::find_if(attributes.begin(), attributes.end(), [&type](const std::shared_ptr<Attribute>& attribute) {return attribute->type == type; });

    if (iterator != attributes.end())
    {
        return *iterator._Ptr;
    }

    return nullptr;
}