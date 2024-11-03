#pragma once

#include <tiny_gltf.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vulkan/vulkan.hpp>

#include <SVMV/Scene.hxx>
#include <SVMV/Node.hxx>
#include <SVMV/Mesh.hxx>
#include <SVMV/Primitive.hxx>
#include <SVMV/Attribute.hxx>

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

            void processMesh(std::shared_ptr<Node> node, const tinygltf::Mesh& mesh);
            void processPrimitive(std::shared_ptr<Node> node, const tinygltf::Primitive& gltfPrimitive);
        }
    }
}

