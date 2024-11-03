#include <SVMV/Loader.hxx>

using namespace SVMV;

namespace
{
    std::shared_ptr<Scene> scene;
    std::shared_ptr<Node> rootNode;
    std::unordered_set<unsigned> meshMap;

    std::shared_ptr<tinygltf::Model> gltfModel;
}

std::shared_ptr<Scene> Loader::loadScene(const std::string& filePath)
{
    scene = std::make_shared<Scene>();
    rootNode = scene->root = std::make_shared<Node>();

    tinygltf::TinyGLTF gltfContext;

    gltfModel = std::make_shared<tinygltf::Model>();
    std::string error;
    std::string warning;

    bool result = gltfContext.LoadASCIIFromFile(gltfModel.get(), &error, &warning, filePath);

    if (!warning.empty())
    {
        std::cout << "tinygltf: warning: " + warning << std::endl;
    }
    if (!error.empty())
    {
        std::cout << "tinygltf: error: " + error << std::endl;
    }
    if (!result)
    {
        throw std::runtime_error("tinigltf: failed to load file: " + filePath);
    }

    details::processScene();

    rootNode.reset();
    meshMap.clear();
    gltfModel.reset();

    return scene;
}

void Loader::details::processScene()
{
    processNodeHierarchy(scene->root, gltfModel->scenes[0].nodes);
}

void Loader::details::processNodeHierarchy(std::shared_ptr<Node> parentNode, const std::vector<int>& children)
{
    for (int index : children)
    {
        processNodeHierarchy(processNode(parentNode, gltfModel->nodes[index]), gltfModel->nodes[index].children);
    }
}

std::shared_ptr<Node> Loader::details::processNode(std::shared_ptr<Node> parentNode, const tinygltf::Node& gltfNode)
{
    std::shared_ptr<Node> node = std::make_shared<Node>();
    parentNode->children.emplace_back(node);

    if (gltfNode.matrix.size() != 0)
    {
        node->transform = glm::make_mat4(gltfNode.matrix.data()); // TODO: test this
    }

    if (gltfNode.mesh != -1)
    {
        node->mesh = std::make_shared<Mesh>();
        processMesh(node, gltfModel->meshes[gltfNode.mesh]);
        scene->meshes.emplace_back(node->mesh);
    }

    return node;
}

void Loader::details::processMesh(std::shared_ptr<Node> node, const tinygltf::Mesh& mesh)
{
    for (const auto& primitive : mesh.primitives)
    {
        processPrimitive(node, primitive);
    }
}

void Loader::details::processPrimitive(std::shared_ptr<Node> node, const tinygltf::Primitive& gltfPrimitive)
{
    std::shared_ptr<Primitive> primitive = std::make_shared<Primitive>();
    node->mesh->primitives.emplace_back(primitive);

    primitive->material = nullptr;

    if (gltfPrimitive.indices != -1)
    {
        tinygltf::Accessor gltfIndices = gltfModel->accessors[gltfPrimitive.indices];
        tinygltf::BufferView gltfBufferView = gltfModel->bufferViews[gltfIndices.bufferView];

        primitive->indices.reserve(gltfIndices.count);

        switch (gltfIndices.componentType)
        {
        case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT:
        {
            uint32_t* data = reinterpret_cast<uint32_t*>(gltfModel->buffers[gltfBufferView.buffer].data.data() + gltfIndices.byteOffset + gltfBufferView.byteOffset);
            for (int i = 0; i < gltfIndices.count; i++)
            {
                primitive->indices.push_back(data[i]);
            }
        }
            break;

        case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT:
        {
            uint16_t* data = reinterpret_cast<uint16_t*>(gltfModel->buffers[gltfBufferView.buffer].data.data() + gltfIndices.byteOffset + gltfBufferView.byteOffset);
            for (int i = 0; i < gltfIndices.count; i++)
            {
                primitive->indices.push_back(data[i]);
            }
        }
            break;

        case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE:
        {
            uint8_t* data = reinterpret_cast<uint8_t*>(gltfModel->buffers[gltfBufferView.buffer].data.data() + gltfIndices.byteOffset + gltfBufferView.byteOffset);
            for (int i = 0; i < gltfIndices.count; i++)
            {
                primitive->indices.push_back(data[i]);
            }
        }
            break;

        default:
            throw std::runtime_error("loader: attempting to load primitive with invalid index type");
            return;
        }
    }

    if (gltfPrimitive.attributes.find("POSITION") != gltfPrimitive.attributes.end()) // TODO: handle special case for when there are no indices
    {
        tinygltf::Accessor gltfAttribute = gltfModel->accessors[gltfPrimitive.attributes.at("POSITION")]; // TODO: make sure the index here isn't -1?
        tinygltf::BufferView gltfBufferView = gltfModel->bufferViews[gltfAttribute.bufferView];

        std::shared_ptr<Attribute> attribute = std::make_shared<Attribute>();
        attribute->type = Attribute::AttributeType::POSITION;
        attribute->count = gltfAttribute.count;
        attribute->components = tinygltf::GetNumComponentsInType(gltfAttribute.type);
        attribute->componentSize = tinygltf::GetComponentSizeInBytes(gltfAttribute.componentType);
        attribute->stride = gltfBufferView.byteStride; // what to do with this?

        attribute->data.resize(gltfAttribute.count * (tinygltf::GetNumComponentsInType(gltfAttribute.type) + 1) * tinygltf::GetComponentSizeInBytes(gltfAttribute.componentType));

        float* data = reinterpret_cast<float*>(gltfModel->buffers[gltfBufferView.buffer].data.data() + gltfAttribute.byteOffset + gltfBufferView.byteOffset);
        float* attributeData = reinterpret_cast<float*>(attribute->data.data());

        int attributePointer = 0;
        int dataPointer = 0;
        for (int i = 0; i < gltfAttribute.count; i++) // one extra float for padding
        {
            attributeData[attributePointer] = data[dataPointer];
            attributeData[attributePointer + 1] = data[dataPointer + 1];
            attributeData[attributePointer + 2] = data[dataPointer + 2];
            attributeData[attributePointer + 3] = 0.0f;

            attributePointer += 4;
            dataPointer += 3;
        }

        primitive->attributes.emplace_back(attribute);
    }

    // TODO: gltf color attributes can be either vec3 or vec4, make sure to convert them all to vec4
}
