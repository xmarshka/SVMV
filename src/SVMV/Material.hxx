#pragma once

#include <glm/glm.hpp>

#include <array>
#include <memory>

namespace SVMV
{
    class Texture;

    class Material // a GLTF PBR material 
    {
    public:
        glm::fvec4 baseColorFactor;
        float metallicFactor;
        float roughnessFactor;

        std::shared_ptr<Texture> diffuseTexture;
        std::shared_ptr<Texture> metallicRoughnessTexture;
        std::shared_ptr<Texture> normalTexture;
        std::shared_ptr<Texture> occlusionTexture;
        std::shared_ptr<Texture> emissiveTexture;
    };
}