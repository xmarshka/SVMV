#include <SVMV/VulkanGLTFPBRMaterial.hxx>
#include <vk_mem_alloc.h>

using namespace SVMV;

GLTFPBRMaterial::GLTFPBRMaterial(
    vk::raii::Device* device, VmaAllocator memoryAllocator, VulkanUtilities::ImmediateSubmit* immediateSubmit, const vk::raii::RenderPass& renderPass, const vk::raii::DescriptorSetLayout& globalDescriptorSetLayout,
    const vk::raii::DescriptorSetLayout& lightDescriptorSetLayout, VulkanUtilities::DescriptorAllocator* descriptorAllocator, VulkanDescriptorWriter* descriptorWriter, const shaderc::Compiler& compiler
)
    : _device(device), _memoryAllocator(memoryAllocator), _immediateSubmit(immediateSubmit), _descriptorAllocator(descriptorAllocator), _descriptorWriter(descriptorWriter)
{
    _vertexShader = VulkanShader(*_device, compiler, VulkanShader::ShaderType::VERTEX, "gltf_pbr_vert.glsl");
    _fragmentShader = VulkanShader(*_device, compiler, VulkanShader::ShaderType::FRAGMENT, "gltf_pbr_frag.glsl");

    vk::DescriptorSetLayoutBinding descriptorSetLayoutBingings[6];
    descriptorSetLayoutBingings[0].setBinding(0);
    descriptorSetLayoutBingings[0].setDescriptorCount(1);
    descriptorSetLayoutBingings[0].setDescriptorType(vk::DescriptorType::eUniformBuffer);
    descriptorSetLayoutBingings[0].setStageFlags(vk::ShaderStageFlagBits::eFragment);

    // base color texture
    descriptorSetLayoutBingings[1].setBinding(1);
    descriptorSetLayoutBingings[1].setDescriptorCount(1);
    descriptorSetLayoutBingings[1].setDescriptorType(vk::DescriptorType::eCombinedImageSampler);
    descriptorSetLayoutBingings[1].setStageFlags(vk::ShaderStageFlagBits::eFragment);

    // normal texture
    descriptorSetLayoutBingings[2].setBinding(2);
    descriptorSetLayoutBingings[2].setDescriptorCount(1);
    descriptorSetLayoutBingings[2].setDescriptorType(vk::DescriptorType::eCombinedImageSampler);
    descriptorSetLayoutBingings[2].setStageFlags(vk::ShaderStageFlagBits::eFragment);

    // metallic roughness texture
    descriptorSetLayoutBingings[3].setBinding(3);
    descriptorSetLayoutBingings[3].setDescriptorCount(1);
    descriptorSetLayoutBingings[3].setDescriptorType(vk::DescriptorType::eCombinedImageSampler);
    descriptorSetLayoutBingings[3].setStageFlags(vk::ShaderStageFlagBits::eFragment);

    // occlusion texture
    descriptorSetLayoutBingings[4].setBinding(4);
    descriptorSetLayoutBingings[4].setDescriptorCount(1);
    descriptorSetLayoutBingings[4].setDescriptorType(vk::DescriptorType::eCombinedImageSampler);
    descriptorSetLayoutBingings[4].setStageFlags(vk::ShaderStageFlagBits::eFragment);

    // emissive texture
    descriptorSetLayoutBingings[5].setBinding(5);
    descriptorSetLayoutBingings[5].setDescriptorCount(1);
    descriptorSetLayoutBingings[5].setDescriptorType(vk::DescriptorType::eCombinedImageSampler);
    descriptorSetLayoutBingings[5].setStageFlags(vk::ShaderStageFlagBits::eFragment);

    vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo;
    descriptorSetLayoutCreateInfo.setBindings(descriptorSetLayoutBingings);

    _descriptorSetLayout = vk::raii::DescriptorSetLayout(*_device, descriptorSetLayoutCreateInfo);

    vk::PushConstantRange pushConstantRange;
    pushConstantRange.setOffset(0);
    pushConstantRange.setSize(sizeof(ShaderStructures::PushConstants));
    pushConstantRange.setStageFlags(vk::ShaderStageFlagBits::eVertex);

    std::vector<vk::DescriptorSetLayout> descriptorSetLayouts = { *globalDescriptorSetLayout, *lightDescriptorSetLayout, *_descriptorSetLayout };

    vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo;
    pipelineLayoutCreateInfo.setPushConstantRanges(pushConstantRange);
    pipelineLayoutCreateInfo.setSetLayouts(descriptorSetLayouts);

    _pipelineLayout = vk::raii::PipelineLayout(*_device, pipelineLayoutCreateInfo);

    _pipeline = VulkanUtilities::createPipeline(*device, _pipelineLayout, renderPass, _vertexShader.getModule(), _fragmentShader.getModule());

    createDefaultResources();
}

GLTFPBRMaterial::GLTFPBRMaterial(GLTFPBRMaterial&& other) noexcept
{
    this->_device = other._device;
    this->_memoryAllocator = std::move(other._memoryAllocator);
    this->_immediateSubmit = other._immediateSubmit;
    this->_descriptorAllocator = other._descriptorAllocator;
    this->_descriptorWriter = other._descriptorWriter;

    this->_pipeline = std::move(other._pipeline);
    this->_pipelineLayout = std::move(other._pipelineLayout);
    this->_descriptorSetLayout = std::move(other._descriptorSetLayout);

    this->_vertexShader = std::move(other._vertexShader);
    this->_fragmentShader = std::move(other._fragmentShader);

    this->_resources = std::move(other._resources);
    this->_descriptorSets = std::move(other._descriptorSets);

    this->_defaultSampler = std::move(other._defaultSampler);
    this->_defaultBaseColorImage = std::move(other._defaultBaseColorImage);
    this->_defaultNormalImage = std::move(other._defaultNormalImage);
    this->_defaultMetallicRoughnessImage = std::move(other._defaultMetallicRoughnessImage);
    this->_defaultOcclusionImage = std::move(other._defaultOcclusionImage);
    this->_defaultEmissiveImage = std::move(other._defaultEmissiveImage);

    other._device = nullptr;
    other._immediateSubmit = nullptr;
    other._descriptorAllocator = nullptr;
    other._descriptorWriter = nullptr;
}

GLTFPBRMaterial& GLTFPBRMaterial::operator=(GLTFPBRMaterial&& other) noexcept
{
    if (this != &other)
    {
        this->_device = other._device;
        this->_memoryAllocator = std::move(other._memoryAllocator);
        this->_immediateSubmit = other._immediateSubmit;
        this->_descriptorAllocator = other._descriptorAllocator;
        this->_descriptorWriter = other._descriptorWriter;

        this->_pipeline = std::move(other._pipeline);
        this->_pipelineLayout = std::move(other._pipelineLayout);
        this->_descriptorSetLayout = std::move(other._descriptorSetLayout);

        this->_vertexShader = std::move(other._vertexShader);
        this->_fragmentShader = std::move(other._fragmentShader);

        this->_resources = std::move(other._resources);
        this->_descriptorSets = std::move(other._descriptorSets);

        this->_defaultSampler = std::move(other._defaultSampler);
        this->_defaultBaseColorImage = std::move(other._defaultBaseColorImage);
        this->_defaultNormalImage = std::move(other._defaultNormalImage);
        this->_defaultMetallicRoughnessImage = std::move(other._defaultMetallicRoughnessImage);
        this->_defaultOcclusionImage = std::move(other._defaultOcclusionImage);
        this->_defaultEmissiveImage = std::move(other._defaultEmissiveImage);

        other._device = nullptr;
        other._immediateSubmit = nullptr;
        other._descriptorAllocator = nullptr;
        other._descriptorWriter = nullptr;
    }
    
    return *this;
}

vk::DescriptorSet GLTFPBRMaterial::createDescriptorSet(std::shared_ptr<Material> material)
{
    vk::raii::DescriptorSet descriptorSet = _descriptorAllocator->allocateSet(_descriptorSetLayout);
    MaterialResources resources;

    processUniformParameters(descriptorSet, resources, material);
    processTextures(descriptorSet, resources, material);

    _resources.push_back(std::move(resources));
    _descriptorSets.push_back(std::move(descriptorSet));

    return *_descriptorSets.back();
}

const vk::raii::Pipeline* GLTFPBRMaterial::getPipeline() const
{
    return &_pipeline;
}

const vk::raii::PipelineLayout* GLTFPBRMaterial::getPipelineLayout() const
{
    return &_pipelineLayout;
}

void GLTFPBRMaterial::createDefaultResources()
{
    uint32_t dimension = 32; // arbitrary small dimensions

    std::unique_ptr<std::byte[]> data = std::make_unique_for_overwrite<std::byte[]>(dimension * dimension * 4); // RGBA

    for (int i = 0; i < dimension * dimension * 4; i++)
    {
        reinterpret_cast<uint8_t*>(data.get())[i] = 255;
    }

    _defaultBaseColorImage = VulkanImage(
        _device, _immediateSubmit, _memoryAllocator, vk::Extent2D{ dimension, dimension },
        vk::Format::eR8G8B8A8Srgb, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, data.get(),
        dimension* dimension * 4
    );

    _defaultMetallicRoughnessImage = VulkanImage(
        _device, _immediateSubmit, _memoryAllocator, vk::Extent2D{ dimension, dimension },
        vk::Format::eR8G8B8A8Unorm, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, data.get(),
        dimension* dimension * 4
    );

    _defaultMetallicRoughnessImage = VulkanImage(
        _device, _immediateSubmit, _memoryAllocator, vk::Extent2D{ dimension, dimension },
        vk::Format::eR8G8B8A8Unorm, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, data.get(),
        dimension* dimension * 4
    );

    _defaultOcclusionImage = VulkanImage(
        _device, _immediateSubmit, _memoryAllocator, vk::Extent2D{ dimension, dimension },
        vk::Format::eR8G8B8A8Unorm, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, data.get(),
        dimension* dimension * 4
    );

    for (int i = 0; i < dimension * dimension * 4; i++)
    {
        reinterpret_cast<uint8_t*>(data.get())[i] = 0;
    }

    _defaultEmissiveImage = VulkanImage(
        _device, _immediateSubmit, _memoryAllocator, vk::Extent2D{ dimension, dimension },
        vk::Format::eR8G8B8A8Srgb, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, data.get(),
        dimension* dimension * 4
    );

    _defaultNormalImage = VulkanImage(
        _device, _immediateSubmit, _memoryAllocator, vk::Extent2D{ dimension, dimension },
        vk::Format::eR8G8B8A8Unorm, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, data.get(),
        dimension * dimension * 4
    );

    vk::SamplerCreateInfo samplerCreateInfo;
    samplerCreateInfo.setMagFilter(vk::Filter::eLinear);
    samplerCreateInfo.setMinFilter(vk::Filter::eLinear);
    samplerCreateInfo.setAddressModeU(vk::SamplerAddressMode::eRepeat);
    samplerCreateInfo.setAddressModeV(vk::SamplerAddressMode::eRepeat);
    samplerCreateInfo.setAddressModeW(vk::SamplerAddressMode::eRepeat);
    samplerCreateInfo.setAnisotropyEnable(vk::False);
    samplerCreateInfo.setMaxAnisotropy(4);
    samplerCreateInfo.setBorderColor(vk::BorderColor::eIntOpaqueBlack);
    samplerCreateInfo.setUnnormalizedCoordinates(vk::False);
    samplerCreateInfo.setCompareEnable(vk::False);
    samplerCreateInfo.setCompareOp(vk::CompareOp::eAlways);
    samplerCreateInfo.setMipmapMode(vk::SamplerMipmapMode::eLinear);

    _defaultSampler = vk::raii::Sampler(*_device, samplerCreateInfo);
}

void GLTFPBRMaterial::processUniformParameters(vk::raii::DescriptorSet& descriptorSet, MaterialResources& resources, std::shared_ptr<Material> material)
{
    GLTFPBRMaterial::MaterialUniformParameters uniformParameters;

    if (material->properties.contains("baseColorFactor"))
    {
        try
        {
            FloatVector4Property* baseColorFactorProperty = dynamic_cast<FloatVector4Property*>(material->properties["baseColorFactor"].get());
            uniformParameters.baseColorFactor = baseColorFactorProperty->data;
        }
        catch (std::bad_cast)
        {
            uniformParameters.baseColorFactor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        }
    }
    else
    {
        uniformParameters.baseColorFactor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    }
    if (material->properties.contains("emissiveFactor"))
    {
        try
        {
            FloatVector4Property* emissiveFactorProperty = dynamic_cast<FloatVector4Property*>(material->properties["emissiveFactor"].get());
            uniformParameters.emissiveFactor = emissiveFactorProperty->data;
        }
        catch (std::bad_cast)
        {
            uniformParameters.baseColorFactor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        }
    }
    else
    {
        uniformParameters.baseColorFactor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    }
    if (material->properties.contains("rougnessFactor"))
    {
        try
        {
            FloatProperty* rougnessFactorProperty = dynamic_cast<FloatProperty*>(material->properties["rougnessFactor"].get());
            uniformParameters.roughnessMetallicNormalFactors.g = rougnessFactorProperty->data;
        }
        catch (std::bad_cast)
        {
            uniformParameters.roughnessMetallicNormalFactors.g = 1.0f;
        }
    }
    else
    {
        uniformParameters.roughnessMetallicNormalFactors.g = 1.0f;
    }
    if (material->properties.contains("metallicFactor"))
    {
        try
        {
            FloatProperty* metallicFactorProperty = dynamic_cast<FloatProperty*>(material->properties["metallicFactor"].get());
            uniformParameters.roughnessMetallicNormalFactors.b = metallicFactorProperty->data;
        }
        catch (std::bad_cast)
        {
            uniformParameters.roughnessMetallicNormalFactors.b = 1.0f;
        }
    }
    else
    {
        uniformParameters.roughnessMetallicNormalFactors.b = 1.0f;
    }

    if (material->properties.contains("normalTexture"))
    {
        uniformParameters.roughnessMetallicNormalFactors.r = 0.0f;
    }
    else
    {
        uniformParameters.roughnessMetallicNormalFactors.r = 1.0f;
    }

    resources.uniformBuffer = VulkanUniformBuffer(_device, _memoryAllocator, sizeof(GLTFPBRMaterial::MaterialUniformParameters));
    resources.uniformBuffer.setData(&uniformParameters, sizeof(GLTFPBRMaterial::MaterialUniformParameters));

    _descriptorWriter->writeBuffer(descriptorSet, resources.uniformBuffer, 0, 0, sizeof(GLTFPBRMaterial::MaterialUniformParameters), vk::DescriptorType::eUniformBuffer);
}

void GLTFPBRMaterial::processTextures(vk::raii::DescriptorSet& descriptorSet, MaterialResources& resources, std::shared_ptr<Material> material)
{
    if (material->properties.contains("baseColorTexture"))
    {
        try
        {
            TextureProperty* textureProperty = dynamic_cast<TextureProperty*>(material->properties["baseColorTexture"].get());
            processCombinedImageSampler(descriptorSet, 1, resources.baseColorImage, resources.baseColorSampler, textureProperty, vk::Format::eR8G8B8A8Srgb);
        }
        catch (std::bad_cast)
        {
            _descriptorWriter->writeImageAndSampler(descriptorSet, _defaultBaseColorImage, vk::ImageLayout::eShaderReadOnlyOptimal, _defaultSampler, 1);
        }
    }
    else
    {
        _descriptorWriter->writeImageAndSampler(descriptorSet, _defaultBaseColorImage, vk::ImageLayout::eShaderReadOnlyOptimal, _defaultSampler, 1);
    }

    if (material->properties.contains("normalTexture"))
    {
        try
        {
            TextureProperty* textureProperty = dynamic_cast<TextureProperty*>(material->properties["normalTexture"].get());
            processCombinedImageSampler(descriptorSet, 2, resources.normalImage, resources.normalSampler, textureProperty, vk::Format::eR8G8B8A8Unorm);
        }
        catch (std::bad_cast)
        {
            _descriptorWriter->writeImageAndSampler(descriptorSet, _defaultNormalImage, vk::ImageLayout::eShaderReadOnlyOptimal, _defaultSampler, 2);
        }
    }
    else
    {
        _descriptorWriter->writeImageAndSampler(descriptorSet, _defaultNormalImage, vk::ImageLayout::eShaderReadOnlyOptimal, _defaultSampler, 2);
    }

    if (material->properties.contains("metallicRoughnessTexture"))
    {
        try
        {
            TextureProperty* textureProperty = dynamic_cast<TextureProperty*>(material->properties["metallicRoughnessTexture"].get());
            processCombinedImageSampler(descriptorSet, 3, resources.metallicRoughnessImage, resources.metallicRoughnessSampler, textureProperty, vk::Format::eR8G8B8A8Unorm);
        }
        catch (std::bad_cast)
        {
            _descriptorWriter->writeImageAndSampler(descriptorSet, _defaultMetallicRoughnessImage, vk::ImageLayout::eShaderReadOnlyOptimal, _defaultSampler, 3);
        }
    }
    else
    {
        _descriptorWriter->writeImageAndSampler(descriptorSet, _defaultMetallicRoughnessImage, vk::ImageLayout::eShaderReadOnlyOptimal, _defaultSampler, 3);
    }

    if (material->properties.contains("occlusionTexture"))
    {
        try
        {
            TextureProperty* textureProperty = dynamic_cast<TextureProperty*>(material->properties["occlusionTexture"].get());
            processCombinedImageSampler(descriptorSet, 4, resources.occlusionImage, resources.occlusionSampler, textureProperty, vk::Format::eR8G8B8A8Unorm);
        }
        catch (std::bad_cast)
        {
            _descriptorWriter->writeImageAndSampler(descriptorSet, _defaultOcclusionImage, vk::ImageLayout::eShaderReadOnlyOptimal, _defaultSampler, 4);
        }
    }
    else
    {
        _descriptorWriter->writeImageAndSampler(descriptorSet, _defaultOcclusionImage, vk::ImageLayout::eShaderReadOnlyOptimal, _defaultSampler, 4);
    }

    if (material->properties.contains("emissiveTexture"))
    {
        try
        {
            TextureProperty* textureProperty = dynamic_cast<TextureProperty*>(material->properties["emissiveTexture"].get());
            processCombinedImageSampler(descriptorSet, 5, resources.emissiveImage, resources.emissiveSampler, textureProperty, vk::Format::eR8G8B8A8Srgb);
        }
        catch (std::bad_cast)
        {
            _descriptorWriter->writeImageAndSampler(descriptorSet, _defaultEmissiveImage, vk::ImageLayout::eShaderReadOnlyOptimal, _defaultSampler, 5);
        }
    }
    else
    {
        _descriptorWriter->writeImageAndSampler(descriptorSet, _defaultEmissiveImage, vk::ImageLayout::eShaderReadOnlyOptimal, _defaultSampler, 5);
    }
}

void GLTFPBRMaterial::processCombinedImageSampler(vk::raii::DescriptorSet& descriptorSet, int binding, VulkanImage& image, vk::raii::Sampler& sampler, const TextureProperty* textureProperty, vk::Format imageFormat)
{
    image = VulkanImage(
        _device, _immediateSubmit, _memoryAllocator, vk::Extent2D{ textureProperty->data->width, textureProperty->data->height },
        imageFormat, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, textureProperty->data->data.get(),
        textureProperty->data->size
    );

    vk::SamplerCreateInfo samplerCreateInfo;
    samplerCreateInfo.setMagFilter(vk::Filter::eLinear);
    samplerCreateInfo.setMinFilter(vk::Filter::eLinear);
    samplerCreateInfo.setAddressModeU(vk::SamplerAddressMode::eRepeat);
    samplerCreateInfo.setAddressModeV(vk::SamplerAddressMode::eRepeat);
    samplerCreateInfo.setAddressModeW(vk::SamplerAddressMode::eRepeat);
    samplerCreateInfo.setAnisotropyEnable(vk::False);
    samplerCreateInfo.setMaxAnisotropy(4);
    samplerCreateInfo.setBorderColor(vk::BorderColor::eIntOpaqueBlack);
    samplerCreateInfo.setUnnormalizedCoordinates(vk::False);
    samplerCreateInfo.setCompareEnable(vk::False);
    samplerCreateInfo.setCompareOp(vk::CompareOp::eAlways);
    samplerCreateInfo.setMipmapMode(vk::SamplerMipmapMode::eLinear);

    sampler = vk::raii::Sampler(*_device, samplerCreateInfo);

    _descriptorWriter->writeImageAndSampler(descriptorSet, image, vk::ImageLayout::eShaderReadOnlyOptimal, sampler, binding);
}
