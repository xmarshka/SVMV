#pragma once

#include <glm/glm.hpp>

#include <string>
#include <memory>

namespace SVMV
{
    struct Texture;

    enum class PropertyType
    {
        UNDEFINED, FLOAT, FLOAT_VECTOR_4, TEXTURE
    };

    struct Property
    {
        PropertyType type{ PropertyType::UNDEFINED };
        std::string name;
    };

    struct FloatProperty : public Property
    {
        FloatProperty() : Property() { type = PropertyType::FLOAT; };

        float data{ 0 };
    };

    struct FloatVector4Property : public Property
    {
        FloatVector4Property() : Property() { type = PropertyType::FLOAT_VECTOR_4; };

        glm::vec4 data{ 0.0f };
    };

    struct TextureProperty : public Property
    {
        TextureProperty() : Property() { type = PropertyType::TEXTURE; };

        std::shared_ptr<Texture> data;
    };
}
