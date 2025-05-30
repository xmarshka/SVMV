#include <SVMV/Loader.hxx>

using namespace SVMV;

std::shared_ptr<Scene> Loader::loadScene(const std::string& filePath)
{
    tinygltf::TinyGLTF gltfContext;

    std::shared_ptr<tinygltf::Model> gltfScene = std::make_shared<tinygltf::Model>();
    std::string error;
    std::string warning;

    bool result = gltfContext.LoadASCIIFromFile(gltfScene.get(), &error, &warning, filePath);

    if (!result)
    {
        result = gltfContext.LoadBinaryFromFile(gltfScene.get(), &error, &warning, filePath);
    }

    if (!result)
    {
        if (!warning.empty())
        {
            std::cout << "tinygltf: warning: " + warning << std::endl;
        }
        if (!error.empty())
        {
            std::cout << "tinygltf: error: " + error << std::endl;
            throw std::runtime_error("tinigltf: failed to load file: " + filePath);
        }
    }

    std::shared_ptr<Scene> scene = details::processScene(gltfScene);

    return scene;
}

std::shared_ptr<Scene> Loader::details::processScene(std::shared_ptr<tinygltf::Model> gltfScene)
{
    std::shared_ptr<Scene> scene = std::make_shared<Scene>();

    scene->materials = processMaterials(gltfScene);
    scene->meshes = processMeshes(gltfScene, scene->materials);
    scene->root = std::make_shared<Node>();

    if (gltfScene->defaultScene != -1)
    {
        for (int nodeIndex : gltfScene->scenes[gltfScene->defaultScene].nodes)
        {
            scene->root->children.push_back(processNodeHierarchy(gltfScene, gltfScene->nodes[nodeIndex], scene->meshes));
        }
    }
    else if (gltfScene->scenes.size() > 0)
    {
        for (int nodeIndex : gltfScene->scenes[0].nodes)
        {
            scene->root->children.push_back(processNodeHierarchy(gltfScene, gltfScene->nodes[nodeIndex], scene->meshes));
        }
    }

    return scene;
}

std::vector<std::shared_ptr<Material>> Loader::details::processMaterials(std::shared_ptr<tinygltf::Model> gltfScene)
{
    std::vector<std::shared_ptr<Material>> materials;

    std::vector<std::shared_ptr<Texture>> textures = processTextures(gltfScene);

    for (const auto& gltfMaterial : gltfScene->materials)
    {
        std::shared_ptr<Material> material = std::make_shared<Material>();
        material->materialTypeName = "glTFPBR";
        material->materialName = gltfMaterial.name;

        processAndInsertFloatProperty(material, "metallicFactor", gltfMaterial.pbrMetallicRoughness.metallicFactor);
        processAndInsertFloatProperty(material, "roughnessFactor", gltfMaterial.pbrMetallicRoughness.roughnessFactor);

        processAndInsertFloatVector4Property(material, "baseColorFactor", gltfMaterial.pbrMetallicRoughness.baseColorFactor);
        processAndInsertFloatVector4Property(material, "emissiveFactor", gltfMaterial.emissiveFactor);

        processAndInsertTextureProperty(textures, material, "baseColorTexture", gltfMaterial.pbrMetallicRoughness.baseColorTexture);
        processAndInsertTextureProperty(textures, material, "metallicRoughnessTexture", gltfMaterial.pbrMetallicRoughness.metallicRoughnessTexture);

        processAndInsertTextureProperty(textures, material, "normalTexture", gltfMaterial.normalTexture);
        processAndInsertTextureProperty(textures, material, "occlusionTexture", gltfMaterial.occlusionTexture);
        processAndInsertTextureProperty(textures, material, "emissiveTexture", gltfMaterial.emissiveTexture);

        materials.push_back(material);
    }

    return materials;
}

std::vector<std::shared_ptr<Texture>> Loader::details::processTextures(std::shared_ptr<tinygltf::Model> gltfScene)
{
    std::vector<std::shared_ptr<Texture>> textures;

    // TODO: create placeholder texture for when there is no source

    for (const auto& gltfTexture : gltfScene->textures)
    {
        std::shared_ptr<Texture> texture = std::make_shared<Texture>();

        if (gltfTexture.source != -1)
        {
            tinygltf::Image gltfImage = gltfScene->images[gltfTexture.source];

            texture->width = gltfImage.width;
            texture->height = gltfImage.height;

            texture->data = std::make_unique<std::byte[]>(gltfImage.width * gltfImage.height * 4); // tinyglTF expands images to RGBA by default
            texture->size = gltfImage.width * gltfImage.height * 4;

            memcpy(texture->data.get(), gltfImage.image.data(), gltfImage.width * gltfImage.height * 4);

            std::byte* debug = texture->data.get();
        }

        textures.push_back(texture);
    }

    return textures;
}

void Loader::details::processAndInsertFloatProperty(std::shared_ptr<Material> targetMaterial, const std::string& name, float gltfFloat)
{
    std::shared_ptr<FloatProperty> floatProperty = std::make_shared<FloatProperty>();
    floatProperty->name = name;
    floatProperty->data = gltfFloat;

    targetMaterial->properties[name] = floatProperty;
}

void Loader::details::processAndInsertFloatVector4Property(std::shared_ptr<Material> targetMaterial, const std::string& name, const std::vector<double>& gltfFactor)
{
    std::shared_ptr<FloatVector4Property> factorProperty = std::make_shared<FloatVector4Property>();
    factorProperty->name = name;

    for (int i = 0; (i < gltfFactor.size()) && (i < 4); i++)
    {
        factorProperty->data[i] = gltfFactor[i];
    }

    targetMaterial->properties[name] = factorProperty;
}

void Loader::details::processAndInsertTextureProperty(const std::vector<std::shared_ptr<Texture>>& textures, std::shared_ptr<Material> targetMaterial, const std::string& name, const tinygltf::TextureInfo& gltfTextureInfo)
{
    if (gltfTextureInfo.index >= 0)
    {
        std::shared_ptr<TextureProperty> textureProperty = std::make_shared<TextureProperty>();
        textureProperty->name = name;

        textureProperty->data = textures[gltfTextureInfo.index];

        targetMaterial->properties[name] = textureProperty;
    }
}

void Loader::details::processAndInsertTextureProperty(const std::vector<std::shared_ptr<Texture>>& textures, std::shared_ptr<Material> targetMaterial, const std::string& name, const tinygltf::NormalTextureInfo& gltfTextureInfo)
{
    if (gltfTextureInfo.index >= 0)
    {
        std::shared_ptr<TextureProperty> textureProperty = std::make_shared<TextureProperty>();
        textureProperty->name = name;

        textureProperty->data = textures[gltfTextureInfo.index];

        targetMaterial->properties[name] = textureProperty;
    }
}

void Loader::details::processAndInsertTextureProperty(const std::vector<std::shared_ptr<Texture>>& textures, std::shared_ptr<Material> targetMaterial, const std::string& name, const tinygltf::OcclusionTextureInfo& gltfTextureInfo)
{
    if (gltfTextureInfo.index >= 0)
    {
        std::shared_ptr<TextureProperty> textureProperty = std::make_shared<TextureProperty>();
        textureProperty->name = name;

        textureProperty->data = textures[gltfTextureInfo.index];

        targetMaterial->properties[name] = textureProperty;
    }
}

void Loader::details::generateTangents(std::shared_ptr<Primitive> primitive)
{
    Attribute tangentAttribute;
    tangentAttribute.attributeType = AttributeType::TANGENT;
    tangentAttribute.componentCount = 4;
    tangentAttribute.count = getAttributeByType(primitive.get(), AttributeType::NORMAL)->count;
    tangentAttribute.type = Type::FLOAT;
    tangentAttribute.size = tangentAttribute.count * tangentAttribute.componentCount * sizeof(float);
    tangentAttribute.elements = std::make_unique_for_overwrite<std::byte[]>(tangentAttribute.size);

    primitive->attributes.push_back(std::move(tangentAttribute));

    SMikkTSpaceContext context = {};
    context.m_pUserData = primitive.get();

    SMikkTSpaceInterface interface = {};
    interface.m_getNumFaces = [](const SMikkTSpaceContext* context) -> int
    {
        return ((Primitive*)(context->m_pUserData))->indices.size() / 3;
    };

    interface.m_getNumVerticesOfFace = [](const SMikkTSpaceContext* context, int face) -> int
    {
        return 3;
    };

    interface.m_getPosition = [](const SMikkTSpaceContext* context, float fvPosOut[], const int face, const int vert)
    {
        Primitive* primitive = (Primitive*)(context->m_pUserData);
        Attribute* attribute = getAttributeByType(primitive, AttributeType::POSITION);

        float* positions = (float*)(attribute->elements.get());

        int index = primitive->indices[face * 3 + vert] * attribute->componentCount;

        fvPosOut[0] = positions[index];
        fvPosOut[1] = positions[index + 1];
        fvPosOut[2] = positions[index + 2];
    };

    interface.m_getNormal = [](const SMikkTSpaceContext* context, float fvNormOut[], const int face, const int vert)
    {
        Primitive* primitive = (Primitive*)(context->m_pUserData);
        Attribute* attribute = getAttributeByType(primitive, AttributeType::NORMAL);

        float* normals = (float*)(attribute->elements.get());

        int index = primitive->indices[face * 3 + vert] * attribute->componentCount;

        fvNormOut[0] = normals[index];
        fvNormOut[1] = normals[index + 1];
        fvNormOut[2] = normals[index + 2];
    };

    interface.m_getTexCoord = [](const SMikkTSpaceContext* context, float fvTexcOut[], const int face, const int vert)
    {
        Primitive* primitive = (Primitive*)(context->m_pUserData);
        Attribute* attribute = getAttributeByType(primitive, AttributeType::TEXCOORD_0);

        float* texcoords = (float*)(attribute->elements.get());

        int index = primitive->indices[face * 3 + vert] * attribute->componentCount;

        fvTexcOut[0] = texcoords[index];
        fvTexcOut[1] = texcoords[index + 1];
    };

    interface.m_setTSpaceBasic = [](const SMikkTSpaceContext* context, const float fvTangent[], const float sign, const int face, const int vert)
    {
        Primitive* primitive = (Primitive*)(context->m_pUserData);
        Attribute* attribute = getAttributeByType(primitive, AttributeType::TANGENT);

        float* tangents = (float*)(attribute->elements.get());

        int index = primitive->indices[face * 3 + vert] * attribute->componentCount;

        tangents[index] = fvTangent[0];
        tangents[index + 1] = fvTangent[1];
        tangents[index + 2] = fvTangent[2];
        tangents[index + 3] = sign;
    };

    context.m_pInterface = &interface;

    genTangSpaceDefault(&context);
}

std::vector<std::shared_ptr<Mesh>> Loader::details::processMeshes(std::shared_ptr<tinygltf::Model> gltfScene, const std::vector<std::shared_ptr<Material>>& materials)
{
    std::vector<std::shared_ptr<Mesh>> meshes;

    for (const auto& gltfMesh : gltfScene->meshes)
    {
        std::shared_ptr<Mesh> mesh = std::make_shared<Mesh>();

        mesh->primitives = processPrimitives(gltfScene, gltfMesh, materials);

        meshes.push_back(mesh);
    }

    return meshes;
}

std::vector<std::shared_ptr<Primitive>> Loader::details::processPrimitives(std::shared_ptr<tinygltf::Model> gltfScene, const tinygltf::Mesh& gltfMesh, const std::vector<std::shared_ptr<Material>>& materials)
{
    std::vector<std::shared_ptr<Primitive>> primitives;

    for (const auto& gltfPrimitive : gltfMesh.primitives)
    {
        std::shared_ptr<Primitive> primitive = std::make_shared<Primitive>();

        if (gltfPrimitive.material != -1)
        {
            primitive->material = materials.at(gltfPrimitive.material);
        }
        else
        {
            //primitive->material = createDefaultMaterial();
        }

        if (gltfPrimitive.indices != -1)
        {
            tinygltf::Accessor gltfIndices = gltfScene->accessors[gltfPrimitive.indices];
            tinygltf::BufferView gltfBufferView = gltfScene->bufferViews[gltfIndices.bufferView];

            int byteStride = (gltfBufferView.byteStride == 0) ? tinygltf::GetComponentSizeInBytes(gltfIndices.componentType) : gltfBufferView.byteStride;

            primitive->indices.reserve(gltfIndices.count);

            switch (gltfIndices.componentType)
            {
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
            {
                std::byte* source = reinterpret_cast<std::byte*>(gltfScene->buffers[gltfBufferView.buffer].data.data() + gltfIndices.byteOffset + gltfBufferView.byteOffset);

                for (int i = 0; i < gltfIndices.count; i++)
                {
                    primitive->indices.push_back(*(reinterpret_cast<uint32_t*>(source)));
                    source += byteStride;
                }
            }
            break;

            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
            {
                std::byte* source = reinterpret_cast<std::byte*>(gltfScene->buffers[gltfBufferView.buffer].data.data() + gltfIndices.byteOffset + gltfBufferView.byteOffset);

                for (int i = 0; i < gltfIndices.count; i++)
                {
                    primitive->indices.push_back(*(reinterpret_cast<uint16_t*>(source)));
                    source += byteStride;
                }
            }
            break;

            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
            {
                std::byte* source = reinterpret_cast<std::byte*>(gltfScene->buffers[gltfBufferView.buffer].data.data() + gltfIndices.byteOffset + gltfBufferView.byteOffset);

                for (int i = 0; i < gltfIndices.count; i++)
                {
                    primitive->indices.push_back(*(reinterpret_cast<uint8_t*>(source)));
                    source += byteStride;
                }
            }
            break;

            default:
                throw std::runtime_error("loader: attempting to load primitive with invalid index type");
                break;
            }
        }

        std::array<const char*, 5> attributeNames{ "POSITION", "NORMAL", "TANGENT", "TEXCOORD_0", "COLOR_0" };

        for (const auto& attributeName : attributeNames)
        {
            if (gltfPrimitive.attributes.find(attributeName) != gltfPrimitive.attributes.end())
            {
                tinygltf::Accessor gltfAttribute = gltfScene->accessors[gltfPrimitive.attributes.at(attributeName)]; // TODO: make sure the index here isn't -1?
                tinygltf::BufferView gltfBufferView = gltfScene->bufferViews[gltfAttribute.bufferView];

                int finalComponentCount = (attributeName == "COLOR_0") ? 4 : tinygltf::GetNumComponentsInType(gltfAttribute.type);
                int byteStride = (gltfBufferView.byteStride == 0) ? (tinygltf::GetComponentSizeInBytes(gltfAttribute.componentType) * finalComponentCount) : gltfBufferView.byteStride;

                Attribute attribute;
                attribute.attributeType = convertAttributeName(attributeName);
                attribute.type = Type::FLOAT;
                attribute.size = gltfAttribute.count * finalComponentCount * tinygltf::GetComponentSizeInBytes(gltfAttribute.componentType);
                attribute.count = gltfAttribute.count;
                attribute.componentCount = finalComponentCount;

                attribute.elements = std::make_unique_for_overwrite<std::byte[]>(gltfAttribute.count * finalComponentCount * tinygltf::GetComponentSizeInBytes(gltfAttribute.componentType));

                std::unique_ptr<float[]> denormalized = nullptr;

                std::byte* source = reinterpret_cast<std::byte*>(gltfScene->buffers[gltfBufferView.buffer].data.data() + gltfAttribute.byteOffset + gltfBufferView.byteOffset);

                if (gltfAttribute.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
                {
                    denormalized = getDenormalizedByteAccessorData(reinterpret_cast<uint8_t*>(source), gltfAttribute.count, tinygltf::GetNumComponentsInType(gltfAttribute.type), gltfBufferView.byteStride);
                    source = reinterpret_cast<std::byte*>(denormalized.get());
                    byteStride = sizeof(float) * finalComponentCount;
                }
                else if (gltfAttribute.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
                {
                    denormalized = getDenormalizedShortAccessorData(reinterpret_cast<uint16_t*>(source), gltfAttribute.count, tinygltf::GetNumComponentsInType(gltfAttribute.type), gltfBufferView.byteStride);
                    source = reinterpret_cast<std::byte*>(denormalized.get());
                    byteStride = sizeof(float) * finalComponentCount;
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

                primitive->attributes.push_back(std::move(attribute));
            }
        }

        if (gltfPrimitive.attributes.find("POSITION") != gltfPrimitive.attributes.end()
            && gltfPrimitive.attributes.find("NORMAL") != gltfPrimitive.attributes.end()
            && gltfPrimitive.attributes.find("TEXCOORD_0") != gltfPrimitive.attributes.end())
        {
            if (gltfPrimitive.attributes.find("TANGENT") == gltfPrimitive.attributes.end())
            {
                generateTangents(primitive);
            }

            primitives.push_back(primitive);
        }
        else
        {
            std::cout << "ERROR: Attempting to load a primitive with missing position, normal or texcoord_0 attribute.\n";
        }
    }

    return primitives;
}

std::shared_ptr<Node> Loader::details::processNodeHierarchy(std::shared_ptr<tinygltf::Model> gltfScene, const tinygltf::Node& gltfNode, const std::vector<std::shared_ptr<Mesh>>& meshes)
{
    std::shared_ptr<Node> node = processNode(gltfNode, meshes);

    for (int index : gltfNode.children)
    {
        node->children.push_back(processNodeHierarchy(gltfScene, gltfScene->nodes.at(index), meshes));
    }

    return node;
}

std::shared_ptr<Node> Loader::details::processNode(const tinygltf::Node& gltfNode, const std::vector<std::shared_ptr<Mesh>>& meshes)
{
    std::shared_ptr<Node> node = std::make_shared<Node>();

    if (gltfNode.matrix.size() != 0)
    {
        node->transform = glm::make_mat4(gltfNode.matrix.data());
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
        node->mesh = meshes.at(gltfNode.mesh);
    }

    return node;
}

std::shared_ptr<Material> Loader::details::createDefaultMaterial()
{
    std::shared_ptr<Material> material = std::make_shared<Material>();
    material->materialTypeName = "glTFPBR";

    return material;
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

    for (int i = 0; i < count * componentCount; i += componentCount)
    {
        for (int j = 0; j < componentCount; j++)
        {
            data.get()[i + j] = static_cast<float>(source[j]) / maximum;
        }

        source += byteStride;
    }

    return data;
}

std::unique_ptr<float[]> Loader::details::getDenormalizedShortAccessorData(uint16_t* source, size_t count, size_t componentCount, size_t byteStride)
{
    // TODO: assert byteStride is a multiple of: sizeof(uint16_t) * componentCount 

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

Attribute* Loader::details::getAttributeByType(Primitive* primitive, AttributeType type)
{
    auto iterator = std::find_if(primitive->attributes.begin(), primitive->attributes.end(), [&](const Attribute& attribute) { return attribute.attributeType == type; });

    if (iterator != primitive->attributes.end())
    {
        return &*iterator;
    }
    else
    {
        return nullptr;
    }
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
