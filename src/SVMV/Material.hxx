#pragma once

#include <glm/glm.hpp>

#include <array>
#include <memory>

namespace SVMV
{
    struct Texture;

    struct Material // a GLTF PBR material 
    {
        enum class MaterialType
        {
            UNLIT, GLTFPBR
        };

        MaterialType type{ MaterialType::GLTFPBR };

        glm::vec4 baseColorFactor{ 1.0f };
        float metallicFactor{ 0.0f };
        float roughnessFactor{ 0.0f };

        std::shared_ptr<Texture> diffuseTexture;
        std::shared_ptr<Texture> metallicRoughnessTexture;
        std::shared_ptr<Texture> normalTexture;
        std::shared_ptr<Texture> occlusionTexture;
        std::shared_ptr<Texture> emissiveTexture;
    };
}