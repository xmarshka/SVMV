#pragma once

#include <tiny_gltf.h>
#include <MikkTSpace/mikktspace.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
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
        void appendScene(std::shared_ptr<Scene> scene, std::shared_ptr<Node> node, glm::mat4 appendedSceneTransformOffset = glm::mat4(0.0f)); // TODO

        namespace details
        {
            std::shared_ptr<Scene> processScene(std::shared_ptr<tinygltf::Model> gltfScene);

            std::vector<std::shared_ptr<Material>> processMaterials(std::shared_ptr<tinygltf::Model> gltfScene);

            std::vector<std::shared_ptr<Texture>> processTextures(std::shared_ptr<tinygltf::Model> gltfScene);

            void processAndInsertFloatProperty(std::shared_ptr<Material> targetMaterial, const std::string& name, float gltfFloat);
            void processAndInsertFloatVector4Property(std::shared_ptr<Material> targetMaterial, const std::string& name, const std::vector<double>& gltfFactor);
            void processAndInsertTextureProperty(const std::vector<std::shared_ptr<Texture>>& textures, std::shared_ptr<Material> targetMaterial, const std::string& name, const tinygltf::TextureInfo& gltfTextureInfo);
            void processAndInsertTextureProperty(const std::vector<std::shared_ptr<Texture>>& textures, std::shared_ptr<Material> targetMaterial, const std::string& name, const tinygltf::NormalTextureInfo& gltfTextureInfo);
            void processAndInsertTextureProperty(const std::vector<std::shared_ptr<Texture>>& textures, std::shared_ptr<Material> targetMaterial, const std::string& name, const tinygltf::OcclusionTextureInfo& gltfTextureInfo);

            void generateTangents(std::shared_ptr<Primitive> primitive);

            std::vector<std::shared_ptr<Mesh>> processMeshes(std::shared_ptr<tinygltf::Model> gltfScene, const std::vector<std::shared_ptr<Material>>& materials);
            std::vector<std::shared_ptr<Primitive>> processPrimitives(std::shared_ptr<tinygltf::Model> gltfScene, const tinygltf::Mesh& gltfMesh, const std::vector<std::shared_ptr<Material>>& materials);

            std::shared_ptr<Node> processNodeHierarchy(std::shared_ptr<tinygltf::Model> gltfScene, const tinygltf::Node& gltfNode, const std::vector<std::shared_ptr<Mesh>>& meshes);
            std::shared_ptr<Node> processNode(const tinygltf::Node& gltfNode, const std::vector<std::shared_ptr<Mesh>>& meshes);

            std::shared_ptr<Material> createDefaultMaterial();

            void copyAccessorToDestination(std::byte* source, std::byte* destination, size_t count, size_t componentCount, size_t componentSize, size_t byteStride);
            void copyMismatchedAccessorToDestination(std::byte* source, std::byte* destination, size_t count, size_t sourceComponentCount, size_t destinationComponentCount, void* fillerValue, size_t componentSize, size_t byteStride);

            std::unique_ptr<float[]> getDenormalizedByteAccessorData(uint8_t* source, size_t count, size_t componentCount, size_t byteStride);
            std::unique_ptr<float[]> getDenormalizedShortAccessorData(uint16_t* source, size_t count, size_t componentCount, size_t byteStride);

            Attribute* getAttributeByType(Primitive* primitive, AttributeType type);

            AttributeType convertAttributeName(const std::string& attributeName);
        }
    }
}

