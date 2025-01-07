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
#include <SVMV/Attribute.hxx>
#include <SVMV/Material.hxx>
#include <SVMV/Property.hxx>
#include <SVMV/Texture.hxx>

#include <memory>
#include <string>
#include <iostream>
#include <vector>
#include <array>
#include <unordered_set>

namespace SVMV
{
    namespace Loader
    {
        std::shared_ptr<Scene> loadScene(const std::string& filePath);
        void appendScene(std::shared_ptr<Scene> scene, const std::string& filePath, glm::mat4 appendedSceneTransformOffset = glm::mat4(0.0f)); // TODO

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

            void copyAccessorToDestination(std::byte* source, std::byte* destination, size_t count, size_t componentCount, size_t componentSize, size_t byteStride);
            void copyMismatchedAccessorToDestination(std::byte* source, std::byte* destination, size_t count, size_t sourceComponentCount, size_t destinationComponentCount, void* fillerValue, size_t componentSize, size_t byteStride);

            std::unique_ptr<float[]> getDenormalizedByteAccessorData(uint8_t* source, size_t count, size_t componentCount, size_t byteStride);
            std::unique_ptr<float[]> getDenormalizedShortAccessorData(uint16_t* source, size_t count, size_t componentCount, size_t byteStride);

            AttributeType convertAttributeName(const std::string& attributeName);
        }
    }
}

