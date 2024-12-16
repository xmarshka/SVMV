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

        Attribute attribute;
        attribute.attributeType = AttributeType::INDEX;
        attribute.type = Type::UINT32;
        attribute.size = gltfIndices.count * sizeof(uint32_t);
        attribute.count = gltfIndices.count;
        attribute.componentCount = 1;

        attribute.elements = std::make_unique_for_overwrite<std::byte[]>(gltfIndices.count * sizeof(uint32_t));

        switch (gltfIndices.componentType)
        {
        case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT:
        {
            uint32_t* destination = reinterpret_cast<uint32_t*>(attribute.elements.get());
            std::byte* source = reinterpret_cast<std::byte*>(gltfModel->buffers[gltfBufferView.buffer].data.data() + gltfIndices.byteOffset + gltfBufferView.byteOffset);

            for (int i = 0; i < gltfIndices.count; i++)
            {
                destination[i] = *(reinterpret_cast<uint32_t*>(source));
                source += gltfBufferView.byteStride;
            }
        }
            break;

        case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT:
        {
            uint32_t* destination = reinterpret_cast<uint32_t*>(attribute.elements.get());
            std::byte* source = reinterpret_cast<std::byte*>(gltfModel->buffers[gltfBufferView.buffer].data.data() + gltfIndices.byteOffset + gltfBufferView.byteOffset);

            for (int i = 0; i < gltfIndices.count; i++)
            {
                destination[i] = *(reinterpret_cast<uint16_t*>(source));
                source += gltfBufferView.byteStride;
            }
        }
            break;

        case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE:
        {
            uint32_t* destination = reinterpret_cast<uint32_t*>(attribute.elements.get());
            std::byte* source = reinterpret_cast<std::byte*>(gltfModel->buffers[gltfBufferView.buffer].data.data() + gltfIndices.byteOffset + gltfBufferView.byteOffset);

            for (int i = 0; i < gltfIndices.count; i++)
            {
                destination[i] = *(reinterpret_cast<uint8_t*>(source));
                source += gltfBufferView.byteStride;
            }
        }
            break;

        default:
            throw std::runtime_error("loader: attempting to load primitive with invalid index type");
            return;
        }
    }

    std::array<const char*, 5> attributeNames { "POSITION", "NORMAL", "TANGENT", "TEXCOORD_0", "COLOR_0" };

    for (const auto& attributeName : attributeNames)
    {
        if (gltfPrimitive.attributes.find(attributeName) != gltfPrimitive.attributes.end())
        {
            tinygltf::Accessor gltfAttribute = gltfModel->accessors[gltfPrimitive.attributes.at(attributeName)]; // TODO: make sure the index here isn't -1?
            tinygltf::BufferView gltfBufferView = gltfModel->bufferViews[gltfAttribute.bufferView];

            int finalComponentCount = (attributeName == "COLOR_0") ? 4 : tinygltf::GetNumComponentsInType(gltfAttribute.type);

            Attribute attribute;
            attribute.attributeType = convertAttributeName(attributeName);
            attribute.type = Type::FLOAT;
            attribute.size = gltfAttribute.count * finalComponentCount * tinygltf::GetComponentSizeInBytes(gltfAttribute.componentType);
            attribute.count = gltfAttribute.count;
            attribute.componentCount = finalComponentCount;

            attribute.elements = std::make_unique_for_overwrite<std::byte[]>(gltfAttribute.count * finalComponentCount * tinygltf::GetComponentSizeInBytes(gltfAttribute.componentType));

            std::unique_ptr<float[]> denormalized = nullptr;

            std::byte* source = reinterpret_cast<std::byte*>(gltfModel->buffers[gltfBufferView.buffer].data.data() + gltfAttribute.byteOffset + gltfBufferView.byteOffset);
            int byteStride = gltfBufferView.byteStride;

            if (gltfAttribute.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
            {
                denormalized = getDenormalizedByteAccessorData(reinterpret_cast<uint8_t*>(source), gltfAttribute.count, tinygltf::GetNumComponentsInType(gltfAttribute.type), gltfBufferView.byteStride);
                source = reinterpret_cast<std::byte*>(denormalized.get());
                byteStride = 0;
            }
            else if (gltfAttribute.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
            {
                denormalized = getDenormalizedShortAccessorData(reinterpret_cast<uint16_t*>(source), gltfAttribute.count, tinygltf::GetNumComponentsInType(gltfAttribute.type), gltfBufferView.byteStride);
                source = reinterpret_cast<std::byte*>(denormalized.get());
                byteStride = 0;
            }

            if (finalComponentCount == tinygltf::GetNumComponentsInType(gltfAttribute.type))
            {
                copyAccessorToDestination(
                    source, attribute.elements.get(), gltfAttribute.count, tinygltf::GetNumComponentsInType(gltfAttribute.type),
                    tinygltf::GetComponentSizeInBytes(gltfAttribute.componentType), byteStride
                );
            }
            else
            {
                float fillerValue = 1.0f; // for color attributes, where the filled alpha value is 1.0f
                copyMismatchedAccessorToDestination(
                    source, attribute.elements.get(), gltfAttribute.count, tinygltf::GetNumComponentsInType(gltfAttribute.type),
                    finalComponentCount, &fillerValue, tinygltf::GetComponentSizeInBytes(gltfAttribute.componentType), byteStride
                );
            }

            primitive->attributes.push_back(attribute);
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

void Loader::details::copyAccessorToDestination(std::byte* source, std::byte* destination, size_t count, size_t componentCount, size_t componentSize, size_t byteStride)
{
    for (int i = 0; i < count; i++)
    {
        memcpy(destination, source, componentCount * componentSize);

        destination += componentCount * componentSize;
        source += byteStride;
    }
}

void Loader::details::copyMismatchedAccessorToDestination(std::byte* source, std::byte* destination, size_t count, size_t sourceComponentCount, size_t destinationComponentCount, void* fillerValue, size_t componentSize, size_t byteStride)
{
    for (int i = 0; i < count; i++)
    {
        memcpy(destination, source, sourceComponentCount * componentSize);
        destination += sourceComponentCount * componentSize;

        for (int i = 0; i < (destinationComponentCount - sourceComponentCount); i++)
        {
            memcpy(destination, fillerValue, 1);
            destination += 1;
        }

        source += byteStride;
    }
}

std::unique_ptr<float[]> Loader::details::getDenormalizedByteAccessorData(uint8_t* source, size_t count, size_t componentCount, size_t byteStride)
{
    std::unique_ptr<float[]> data = std::make_unique_for_overwrite<float[]>(count * componentCount);

    size_t maximum = std::powl(2, sizeof(uint8_t) * 8) - 1;

    for (int i = 0; i < count; i++)
    {
        for (int j = 0; j < componentCount; j++)
        {
            data.get()[i] = static_cast<float>(source[j]) / maximum;
        }

        source += byteStride;
    }

    return data;
}

std::unique_ptr<float[]> Loader::details::getDenormalizedShortAccessorData(uint16_t* source, size_t count, size_t componentCount, size_t byteStride)
{
    std::unique_ptr<float[]> data = std::make_unique_for_overwrite<float[]>(count * componentCount);

    size_t maximum = std::powl(2, sizeof(uint16_t) * 8) - 1;

    for (int i = 0; i < count * componentCount; i += componentCount)
    {
        for (int j = 0; j < componentCount; j++)
        {
            data.get()[i + j] = static_cast<float>(source[j]) / maximum;
        }

        source = reinterpret_cast<uint16_t*>((reinterpret_cast<uint8_t*>(source) + byteStride));
    }

    return data;
}

AttributeType Loader::details::convertAttributeName(const std::string& attributeName)
{
    if (attributeName == "POSITION")
    {
        return AttributeType::POSITION;
    }
    else if (attributeName == "NORMAL")
    {
        return AttributeType::NORMAL;
    }
    else if (attributeName == "TANGENT")
    {
        return AttributeType::TANGENT;
    }
    else if (attributeName == "TEXCOORD_0")
    {
        return AttributeType::TEXCOORD_0;
    }
    else if (attributeName == "COLOR_0")
    {
        return AttributeType::COLOR_0;
    }
    else
    {
        return AttributeType::UNDEFINED;
    }
}
