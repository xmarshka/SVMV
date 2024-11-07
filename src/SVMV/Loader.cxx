#include <SVMV/Loader.hxx>

using namespace SVMV;

namespace
{
    std::shared_ptr<Scene> scene;
    std::shared_ptr<Node> rootNode;

    std::unordered_map<unsigned int, std::shared_ptr<Mesh>> meshMap;

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
    if (gltfModel->defaultScene != -1)
    {
        processNodeHierarchy(scene->root, gltfModel->scenes[gltfModel->defaultScene].nodes);
    }
    else
    {
        processNodeHierarchy(scene->root, gltfModel->scenes[0].nodes);
    }
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
        if (meshMap.find(gltfNode.mesh) != meshMap.end())
        {
            node->mesh = meshMap[gltfNode.mesh];
        }
        else
        {
            node->mesh = std::make_shared<Mesh>();
            processMesh(node, gltfModel->meshes[gltfNode.mesh]);

            scene->meshes.emplace_back(node->mesh);
            meshMap[gltfNode.mesh] = node->mesh;
        }
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

        std::shared_ptr<Attribute> attribute = std::make_shared<Attribute>(gltfAttribute.count, 3, 1, sizeof(float), Attribute::AttributeType::POSITION); // TODO: move semantics
        attribute->data.resize(gltfAttribute.count * (attribute->components + attribute->padding) * sizeof(float)); // our positions are always in vec4s

        // positions in gltf models are always floats in vec3s
        float* data = reinterpret_cast<float*>(gltfModel->buffers[gltfBufferView.buffer].data.data() + gltfAttribute.byteOffset + gltfBufferView.byteOffset);
        float* attributeData = reinterpret_cast<float*>(attribute->data.data());

        copyAccessorToDestination<float>(data, attributeData, gltfAttribute.count, 3, gltfBufferView.byteStride, 4);

        primitive->attributes.emplace_back(attribute);
    }

    if (gltfPrimitive.attributes.find("NORMAL") != gltfPrimitive.attributes.end()) // TODO: handle special case for when there are no indices
    {
        tinygltf::Accessor gltfAttribute = gltfModel->accessors[gltfPrimitive.attributes.at("NORMAL")]; // TODO: make sure the index here isn't -1?
        tinygltf::BufferView gltfBufferView = gltfModel->bufferViews[gltfAttribute.bufferView];

        std::shared_ptr<Attribute> attribute = std::make_shared<Attribute>(gltfAttribute.count, 3, 1, sizeof(float), Attribute::AttributeType::NORMAL); // TODO: move semantics
        attribute->data.resize(gltfAttribute.count * (attribute->components + attribute->padding) * sizeof(float)); // our positions are always in vec4s

        // normals in gltf models are always floats in vec3s
        float* data = reinterpret_cast<float*>(gltfModel->buffers[gltfBufferView.buffer].data.data() + gltfAttribute.byteOffset + gltfBufferView.byteOffset);
        float* attributeData = reinterpret_cast<float*>(attribute->data.data());

        copyAccessorToDestination<float>(data, attributeData, gltfAttribute.count, 3, gltfBufferView.byteStride, 4);

        primitive->attributes.emplace_back(attribute);
    }

    if (gltfPrimitive.attributes.find("TANGENT") != gltfPrimitive.attributes.end()) // TODO: handle special case for when there are no indices
    {
        tinygltf::Accessor gltfAttribute = gltfModel->accessors[gltfPrimitive.attributes.at("TANGENT")]; // TODO: make sure the index here isn't -1?
        tinygltf::BufferView gltfBufferView = gltfModel->bufferViews[gltfAttribute.bufferView];

        std::shared_ptr<Attribute> attribute = std::make_shared<Attribute>(gltfAttribute.count, 4, 0, sizeof(float), Attribute::AttributeType::TANGENT); // TODO: move semantics
        attribute->data.resize(gltfAttribute.count * (attribute->components + attribute->padding) * sizeof(float)); // our positions are always in vec4s

        // normals in gltf models are always floats in vec3s
        float* data = reinterpret_cast<float*>(gltfModel->buffers[gltfBufferView.buffer].data.data() + gltfAttribute.byteOffset + gltfBufferView.byteOffset);
        float* attributeData = reinterpret_cast<float*>(attribute->data.data());

        copyAccessorToDestination<float>(data, attributeData, gltfAttribute.count, 4, gltfBufferView.byteStride);

        primitive->attributes.emplace_back(attribute);
    }

    if (gltfPrimitive.attributes.find("TEXCOORD_0") != gltfPrimitive.attributes.end()) // TODO: handle special case for when there are no indices
    {
        tinygltf::Accessor gltfAttribute = gltfModel->accessors[gltfPrimitive.attributes.at("TEXCOORD_0")]; // TODO: make sure the index here isn't -1?
        tinygltf::BufferView gltfBufferView = gltfModel->bufferViews[gltfAttribute.bufferView];

        std::shared_ptr<Attribute> attribute = std::make_shared<Attribute>(gltfAttribute.count, 2, 0, sizeof(float), Attribute::AttributeType::TEXCOORD); // TODO: move semantics
        attribute->data.resize(gltfAttribute.count * (attribute->components + attribute->padding) * sizeof(float));

        if (gltfAttribute.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
        {
            float* data = reinterpret_cast<float*>(gltfModel->buffers[gltfBufferView.buffer].data.data() + gltfAttribute.byteOffset + gltfBufferView.byteOffset);
            float* attributeData = reinterpret_cast<float*>(attribute->data.data());

            copyAccessorToDestination<float>(data, attributeData, gltfAttribute.count, 2, gltfBufferView.byteStride);
        }
        else if (gltfAttribute.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
        {
            uint8_t* data = reinterpret_cast<uint8_t*>(gltfModel->buffers[gltfBufferView.buffer].data.data() + gltfAttribute.byteOffset + gltfBufferView.byteOffset);
            float* attributeData = reinterpret_cast<float*>(attribute->data.data());

            copyNormalizedAccessorToDestination<uint8_t>(data, attributeData, gltfAttribute.count, 2, gltfBufferView.byteStride);
        }
        else if (gltfAttribute.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
        {
            uint16_t* data = reinterpret_cast<uint16_t*>(gltfModel->buffers[gltfBufferView.buffer].data.data() + gltfAttribute.byteOffset + gltfBufferView.byteOffset);
            float* attributeData = reinterpret_cast<float*>(attribute->data.data());

            copyNormalizedAccessorToDestination<uint16_t>(data, attributeData, gltfAttribute.count, 2, gltfBufferView.byteStride);
        }

        primitive->attributes.emplace_back(attribute);
    }

    if (gltfPrimitive.attributes.find("COLOR_0") != gltfPrimitive.attributes.end()) // TODO: handle special case for when there are no indices
    {
        tinygltf::Accessor gltfAttribute = gltfModel->accessors[gltfPrimitive.attributes.at("COLOR_0")]; // TODO: make sure the index here isn't -1?
        tinygltf::BufferView gltfBufferView = gltfModel->bufferViews[gltfAttribute.bufferView];

        std::shared_ptr<Attribute> attribute = std::make_shared<Attribute>(gltfAttribute.count, 4, 0, sizeof(float), Attribute::AttributeType::COLOR); // TODO: move semantics
        attribute->data.resize(gltfAttribute.count * (attribute->components + attribute->padding) * sizeof(float));

        if (gltfAttribute.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
        {
            float* data = reinterpret_cast<float*>(gltfModel->buffers[gltfBufferView.buffer].data.data() + gltfAttribute.byteOffset + gltfBufferView.byteOffset);
            float* attributeData = reinterpret_cast<float*>(attribute->data.data());

            if (tinygltf::GetNumComponentsInType(gltfAttribute.type) == 4)
            {
                copyAccessorToDestination<float>(data, attributeData, gltfAttribute.count, 4, gltfBufferView.byteStride);
            }
            else if (tinygltf::GetNumComponentsInType(gltfAttribute.type) == 3)
            {
                copyAccessorToDestination<float>(data, attributeData, gltfAttribute.count, 3, gltfBufferView.byteStride, 4, 1.0f);
            }
        }
        else if (gltfAttribute.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
        {
            uint8_t* data = reinterpret_cast<uint8_t*>(gltfModel->buffers[gltfBufferView.buffer].data.data() + gltfAttribute.byteOffset + gltfBufferView.byteOffset);
            float* attributeData = reinterpret_cast<float*>(attribute->data.data());

            if (tinygltf::GetNumComponentsInType(gltfAttribute.type) == 4)
            {
                copyNormalizedAccessorToDestination<uint8_t>(data, attributeData, gltfAttribute.count, 4, gltfBufferView.byteStride);
            }
            else if (tinygltf::GetNumComponentsInType(gltfAttribute.type) == 3)
            {
                copyNormalizedAccessorToDestination<uint8_t>(data, attributeData, gltfAttribute.count, 3, gltfBufferView.byteStride, 4, 1.0f);
            }
        }
        else if (gltfAttribute.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
        {
            uint16_t* data = reinterpret_cast<uint16_t*>(gltfModel->buffers[gltfBufferView.buffer].data.data() + gltfAttribute.byteOffset + gltfBufferView.byteOffset);
            float* attributeData = reinterpret_cast<float*>(attribute->data.data());

            if (tinygltf::GetNumComponentsInType(gltfAttribute.type) == 4)
            {
                copyNormalizedAccessorToDestination<uint16_t>(data, attributeData, gltfAttribute.count, 4, gltfBufferView.byteStride);
            }
            else if (tinygltf::GetNumComponentsInType(gltfAttribute.type) == 3)
            {
                copyNormalizedAccessorToDestination<uint16_t>(data, attributeData, gltfAttribute.count, 3, gltfBufferView.byteStride, 4, 1.0f);
            }
        }

        primitive->attributes.emplace_back(attribute);
    }
}

template <typename sourceType>
void Loader::details::copyAccessorToDestination(sourceType* source, sourceType* destination, size_t count, size_t componentCount, size_t byteStride)
{
    sourceType* sourcePointer = source;
    sourceType* destinationPointer = destination;

    if (byteStride != 0)
    {
        byteStride -= componentCount * sizeof(sourceType);
    }

    for (int i = 0; i < count; i++)
    {
        memcpy(destinationPointer, sourcePointer, componentCount * sizeof(sourceType));

        sourcePointer += componentCount + (byteStride / sizeof(sourceType)); // advances by sizeof(T) bytes, from the spec: "When byteStride is defined, it MUST be a multiple of the size of the accessor’s component type."
        destinationPointer += componentCount;
    }
}

template <typename sourceType>
void Loader::details::copyAccessorToDestination(sourceType* source, sourceType* destination, size_t count, size_t componentCount, size_t byteStride, size_t destinationComponentCount)
{
    sourceType* sourcePointer = source;
    sourceType* destinationPointer = destination;

    if (byteStride != 0)
    {
        byteStride -= componentCount * sizeof(sourceType);
    }

    for (int i = 0; i < count; i++)
    {
        memcpy(destinationPointer, sourcePointer, componentCount * sizeof(sourceType));

        sourcePointer += componentCount + (byteStride / sizeof(sourceType)); // advances by 4 bytes, from the spec: "When byteStride is defined, it MUST be a multiple of the size of the accessor’s component type."
        destinationPointer += destinationComponentCount;
    }
}

template <typename sourceType>
void Loader::details::copyAccessorToDestination(sourceType* source, sourceType* destination, size_t count, size_t componentCount, size_t byteStride, size_t destinationComponentCount, float fillerValue)
{
    sourceType* sourcePointer = source;
    sourceType* destinationPointer = destination;

    if (byteStride != 0)
    {
        byteStride -= componentCount * sizeof(sourceType);
    }

    for (int i = 0; i < count; i++)
    {
        memcpy(destinationPointer, sourcePointer, componentCount * sizeof(sourceType));

        sourcePointer += componentCount + (byteStride / sizeof(sourceType)); // advances by 4 bytes, from the spec: "When byteStride is defined, it MUST be a multiple of the size of the accessor’s component type."
        destinationPointer += componentCount;

        for (int j = 0; j < destinationComponentCount - componentCount; j++)
        {
            *destinationPointer = fillerValue;
            destinationPointer++;
        }
    }
}

template <typename sourceType>
void Loader::details::copyNormalizedAccessorToDestination(sourceType* source, float* destination, size_t count, size_t componentCount, size_t byteStride)
{
    sourceType* sourcePointer = source;
    float* destinationPointer = destination;

    if (byteStride != 0)
    {
        byteStride -= componentCount * sizeof(sourceType);
    }

    size_t maximum = std::powl(2, sizeof(sourceType) * 8) - 1;

    int i = 0;

    for (; i < componentCount; i++)
    {
        *destinationPointer = static_cast<float>(*sourcePointer) / maximum;

        sourcePointer++;
        destinationPointer++;
    }

    for (; i < count; i++)
    {
        if (i % componentCount == 0)
        {
            sourcePointer += byteStride / sizeof(sourceType);
        }

        *destinationPointer = static_cast<float>(*sourcePointer) / maximum;

        sourcePointer++;
        destinationPointer++;
    }
}

template <typename sourceType>
void Loader::details::copyNormalizedAccessorToDestination(sourceType* source, float* destination, size_t count, size_t componentCount, size_t byteStride, size_t destinationComponentCount, float fillerValue)
{
    sourceType* sourcePointer = source;
    float* destinationPointer = destination;

    if (byteStride != 0)
    {
        byteStride -= componentCount * sizeof(sourceType);
    }

    size_t maximum = std::powl(2, sizeof(sourceType) * 8) - 1;

    int i = 0;

    for (; i < componentCount; i++)
    {
        *destinationPointer = static_cast<float>(*sourcePointer) / maximum;

        sourcePointer++;
        destinationPointer++;
    }

    for (; i < count; i++)
    {
        if (i % componentCount == 0)
        {
            sourcePointer += byteStride / sizeof(sourceType);

            for (int j = 0; j < destinationComponentCount - componentCount; j++)
            {
                *destinationPointer = fillerValue;
                destinationPointer++;
            }
        }

        *destinationPointer = static_cast<float>(*sourcePointer) / maximum;

        sourcePointer++;
        destinationPointer++;
    }
}
