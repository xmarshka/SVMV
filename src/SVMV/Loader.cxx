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
    else
    {
        glm::mat4 scale(1.0f);
        glm::mat4 rotate(1.0f);
        glm::mat4 translate(1.0f);

        if (gltfNode.translation.size() != 0)
        {
            translate = glm::translate(translate, glm::vec3(gltfNode.translation[0], gltfNode.translation[1], gltfNode.translation[2]));
        }
        if (gltfNode.rotation.size() != 0)
        {
            rotate = glm::toMat4(glm::quat(gltfNode.rotation[3], gltfNode.rotation[0], gltfNode.rotation[1], gltfNode.rotation[2]));
        }
        if (gltfNode.scale.size() != 0)
        {
            scale = glm::scale(scale, glm::vec3(gltfNode.scale[0], gltfNode.scale[1], gltfNode.scale[2]));
        }

        node->transform = translate * rotate * scale * node->transform;
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

void Loader::details::processMesh(std::shared_ptr<Node> node, const tinygltf::Mesh& gltfMesh)
{
    for (const auto& primitive : gltfMesh.primitives)
    {
        processPrimitive(node, primitive);
    }
}

void Loader::details::processPrimitive(std::shared_ptr<Node> node, const tinygltf::Primitive& gltfPrimitive)
{
    std::shared_ptr<Primitive> primitive = std::make_shared<Primitive>();
    node->mesh->primitives.emplace_back(primitive);

    if (gltfPrimitive.material != -1)
    {
        processMaterial(primitive, gltfModel->materials[gltfPrimitive.material]);
    }
    else
    {
        primitive->material = createDefaultMaterial();
    }

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

        primitive->positions.resize(gltfAttribute.count);

        // positions in gltf models are always floats in vec3s
        float* data = reinterpret_cast<float*>(gltfModel->buffers[gltfBufferView.buffer].data.data() + gltfAttribute.byteOffset + gltfBufferView.byteOffset);
        float* attributeData = reinterpret_cast<float*>(primitive->positions.data());

        copyAccessorToDestination<float>(data, attributeData, gltfAttribute.count, 3, gltfBufferView.byteStride);
    }

    if (gltfPrimitive.attributes.find("NORMAL") != gltfPrimitive.attributes.end()) // TODO: handle special case for when there are no indices
    {
        tinygltf::Accessor gltfAttribute = gltfModel->accessors[gltfPrimitive.attributes.at("NORMAL")]; // TODO: make sure the index here isn't -1?
        tinygltf::BufferView gltfBufferView = gltfModel->bufferViews[gltfAttribute.bufferView];

        primitive->normals.resize(gltfAttribute.count);

        // normals in gltf models are always floats in vec3s
        float* data = reinterpret_cast<float*>(gltfModel->buffers[gltfBufferView.buffer].data.data() + gltfAttribute.byteOffset + gltfBufferView.byteOffset);
        float* attributeData = reinterpret_cast<float*>(primitive->normals.data());

        copyAccessorToDestination<float>(data, attributeData, gltfAttribute.count, 3, gltfBufferView.byteStride);
    }

    if (gltfPrimitive.attributes.find("TANGENT") != gltfPrimitive.attributes.end()) // TODO: handle special case for when there are no indices
    {
        tinygltf::Accessor gltfAttribute = gltfModel->accessors[gltfPrimitive.attributes.at("TANGENT")]; // TODO: make sure the index here isn't -1?
        tinygltf::BufferView gltfBufferView = gltfModel->bufferViews[gltfAttribute.bufferView];

        primitive->tangens.resize(gltfAttribute.count);

        // normals in gltf models are always floats in vec3s
        float* data = reinterpret_cast<float*>(gltfModel->buffers[gltfBufferView.buffer].data.data() + gltfAttribute.byteOffset + gltfBufferView.byteOffset);
        float* attributeData = reinterpret_cast<float*>(primitive->tangens.data());

        copyAccessorToDestination<float>(data, attributeData, gltfAttribute.count, 4, gltfBufferView.byteStride);
    }

    if (gltfPrimitive.attributes.find("TEXCOORD_0") != gltfPrimitive.attributes.end()) // TODO: handle special case for when there are no indices
    {
        tinygltf::Accessor gltfAttribute = gltfModel->accessors[gltfPrimitive.attributes.at("TEXCOORD_0")]; // TODO: make sure the index here isn't -1?
        tinygltf::BufferView gltfBufferView = gltfModel->bufferViews[gltfAttribute.bufferView];

        primitive->texcoords_0.resize(gltfAttribute.count);

        if (gltfAttribute.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
        {
            float* data = reinterpret_cast<float*>(gltfModel->buffers[gltfBufferView.buffer].data.data() + gltfAttribute.byteOffset + gltfBufferView.byteOffset);
            float* attributeData = reinterpret_cast<float*>(primitive->texcoords_0.data());

            copyAccessorToDestination<float>(data, attributeData, gltfAttribute.count, 2, gltfBufferView.byteStride);
        }
        else if (gltfAttribute.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
        {
            uint8_t* data = reinterpret_cast<uint8_t*>(gltfModel->buffers[gltfBufferView.buffer].data.data() + gltfAttribute.byteOffset + gltfBufferView.byteOffset);
            float* attributeData = reinterpret_cast<float*>(primitive->texcoords_0.data());

            copyNormalizedAccessorToDestination<uint8_t>(data, attributeData, gltfAttribute.count, 2, gltfBufferView.byteStride);
        }
        else if (gltfAttribute.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
        {
            uint16_t* data = reinterpret_cast<uint16_t*>(gltfModel->buffers[gltfBufferView.buffer].data.data() + gltfAttribute.byteOffset + gltfBufferView.byteOffset);
            float* attributeData = reinterpret_cast<float*>(primitive->texcoords_0.data());

            copyNormalizedAccessorToDestination<uint16_t>(data, attributeData, gltfAttribute.count, 2, gltfBufferView.byteStride);
        }
    }

    if (gltfPrimitive.attributes.find("COLOR_0") != gltfPrimitive.attributes.end()) // TODO: handle special case for when there are no indices
    {
        tinygltf::Accessor gltfAttribute = gltfModel->accessors[gltfPrimitive.attributes.at("COLOR_0")]; // TODO: make sure the index here isn't -1?
        tinygltf::BufferView gltfBufferView = gltfModel->bufferViews[gltfAttribute.bufferView];

        primitive->colors_0.resize(gltfAttribute.count);

        if (gltfAttribute.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
        {
            float* data = reinterpret_cast<float*>(gltfModel->buffers[gltfBufferView.buffer].data.data() + gltfAttribute.byteOffset + gltfBufferView.byteOffset);
            float* attributeData = reinterpret_cast<float*>(primitive->colors_0.data());

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
            float* attributeData = reinterpret_cast<float*>(primitive->colors_0.data());

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
            float* attributeData = reinterpret_cast<float*>(primitive->colors_0.data());

            if (tinygltf::GetNumComponentsInType(gltfAttribute.type) == 4)
            {
                copyNormalizedAccessorToDestination<uint16_t>(data, attributeData, gltfAttribute.count, 4, gltfBufferView.byteStride);
            }
            else if (tinygltf::GetNumComponentsInType(gltfAttribute.type) == 3)
            {
                copyNormalizedAccessorToDestination<uint16_t>(data, attributeData, gltfAttribute.count, 3, gltfBufferView.byteStride, 4, 1.0f);
            }
        }
    }
}

void Loader::details::processMaterial(std::shared_ptr<Primitive> primitive, const tinygltf::Material& gltfMaterial)
{
    std::shared_ptr<Material> material = std::make_shared<Material>();

    for (int i = 0; i < material->baseColorFactor.length(); i++)
    {
        material->baseColorFactor[i] = gltfMaterial.pbrMetallicRoughness.baseColorFactor[i];
    }

    material->metallicFactor = gltfMaterial.pbrMetallicRoughness.metallicFactor;
    material->roughnessFactor = gltfMaterial.pbrMetallicRoughness.roughnessFactor;

    if (gltfMaterial.pbrMetallicRoughness.baseColorTexture.index != -1)
    {
        processTexture(material->diffuseTexture, gltfMaterial.pbrMetallicRoughness.baseColorTexture);
    }

    primitive->material = material;
}

std::shared_ptr<Material> Loader::details::createDefaultMaterial()
{
    std::shared_ptr<Material> material = std::make_shared<Material>();

    material->baseColorFactor = glm::fvec4(1.0f);
    material->metallicFactor = 0.0f;
    material->roughnessFactor = 1.0f;

    material->diffuseTexture = nullptr;
    material->metallicRoughnessTexture = nullptr;

    material->normalTexture = nullptr;
    material->occlusionTexture = nullptr;
    material->emissiveTexture = nullptr;

    return material;
}

void Loader::details::processTexture(std::shared_ptr<Texture> texture, const tinygltf::TextureInfo& gltfTextureInfo)
{
    texture = std::make_shared<Texture>();

    // TODO: deal with undefined images and or samplers
    tinygltf::Texture gltfTexture = gltfModel->textures[gltfTextureInfo.index];
    tinygltf::Image gltfImage = gltfModel->images[gltfTexture.source];
    tinygltf::Sampler gltfSampler = gltfModel->samplers[gltfTexture.sampler];

    texture->width = gltfImage.width;
    texture->height = gltfImage.height;

    // TODO: this ought to depend on the image format
    uint8_t* data = reinterpret_cast<uint8_t*>(malloc(gltfImage.width * gltfImage.height * 4));
    texture->data = std::span<uint8_t>(data, gltfImage.width * gltfImage.height * 4);

    if (gltfImage.component = 3)
    {

    }
    else // assumed RGBA
    {
        memcpy(texture->data.data(), gltfImage.image.data(), gltfImage.width * gltfImage.height * 4);
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
