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
        virtual PropertyType getType() { return PropertyType::UNDEFINED; };
        virtual ~Property() = default;

        std::string name;
    };

    struct FloatProperty : public Property
    {
        PropertyType getType() override { return PropertyType::FLOAT; };

        float data{ 0 };
    };

    struct FloatVector4Property : public Property
    {
        PropertyType getType() override { return PropertyType::FLOAT_VECTOR_4; };

        glm::vec4 data{ 0.0f };
    };

    struct TextureProperty : public Property
    {
        PropertyType getType() override { return PropertyType::TEXTURE; };

        std::shared_ptr<Texture> data;
    };
}
