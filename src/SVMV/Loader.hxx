#pragma once

#include <tiny_gltf.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <vulkan/vulkan.hpp>

#include <SVMV/Scene.hxx>
#include <SVMV/Node.hxx>
#include <SVMV/Mesh.hxx>
#include <SVMV/Primitive.hxx>
#include <SVMV/Material.hxx>
#include <SVMV/Texture.hxx>

#include <memory>
#include <string>
#include <iostream>
#include <vector>
#include <unordered_set>

namespace SVMV
{
    namespace Loader
    {
        std::shared_ptr<Scene> loadScene(const std::string& filePath);

        namespace details
        {
            void processScene();
            void processNodeHierarchy(std::shared_ptr<Node> parentNode, const std::vector<int>& children);
            std::shared_ptr<Node> processNode(std::shared_ptr<Node> parentNode, const tinygltf::Node& gltfNode);

            void processMesh(std::shared_ptr<Node> node, const tinygltf::Mesh& gltfMesh);
            void processPrimitive(std::shared_ptr<Node> node, const tinygltf::Primitive& gltfPrimitive);

            void processMaterial(std::shared_ptr<Primitive> primitive, const tinygltf::Material& gltfMaterial);
            std::shared_ptr<Material> createDefaultMaterial();

            void processTexture(std::shared_ptr<Texture> texture, const tinygltf::TextureInfo& gltfTextureInfo);

            template <typename sourceType>
            void copyAccessorToDestination(sourceType* source, sourceType* destination, size_t count, size_t componentCount, size_t byteStride);

            template <typename sourceType>
            void copyAccessorToDestination(sourceType* source, sourceType* destination, size_t count, size_t componentCount, size_t byteStride, size_t destinationComponentCount);

            template <typename sourceType>
            void copyAccessorToDestination(sourceType* source, sourceType* destination, size_t count, size_t componentCount, size_t byteStride, size_t destinationComponentCount, float fillerValue);

            template <typename sourceType>
            void copyNormalizedAccessorToDestination(sourceType* source, float* destination, size_t count, size_t componentCount, size_t byteStride);

            template <typename sourceType>
            void copyNormalizedAccessorToDestination(sourceType* source, float* destination, size_t count, size_t componentCount, size_t byteStride, size_t destinationComponentCount, float fillerValue);
        }
    }
}

