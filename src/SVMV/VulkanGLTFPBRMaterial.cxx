#include <SVMV/VulkanGLTFPBRMaterial.hxx>
#include <vk_mem_alloc.h>

using namespace SVMV;

GLTFPBRMaterial::GLTFPBRMaterial(
    vk::raii::Device* device, VmaAllocator memoryAllocator, const vk::raii::RenderPass& renderPass, const vk::raii::DescriptorSetLayout& globalDescriptorSetLayout,
    VulkanUtilities::DescriptorAllocator* descriptorAllocator, VulkanDescriptorWriter* descriptorWriter, const shaderc::Compiler& compiler
)
    : _device(device), _memoryAllocator(memoryAllocator), _descriptorAllocator(descriptorAllocator), _descriptorWriter(descriptorWriter)
{
    _vertexShader = VulkanShader(*_device, compiler, VulkanShader::ShaderType::VERTEX, "shader.vert");
    _fragmentShader = VulkanShader(*_device, compiler, VulkanShader::ShaderType::FRAGMENT, "shader.frag");
 
    vk::DescriptorSetLayoutBinding descriptorSetLayoutBingings[2];
    descriptorSetLayoutBingings[0].setBinding(0);
    descriptorSetLayoutBingings[0].setDescriptorCount(1);
    descriptorSetLayoutBingings[0].setDescriptorType(vk::DescriptorType::eUniformBuffer);

    descriptorSetLayoutBingings[1].setBinding(1);
    descriptorSetLayoutBingings[1].setDescriptorCount(1);
    descriptorSetLayoutBingings[1].setDescriptorType(vk::DescriptorType::eCombinedImageSampler);

    vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo;
    descriptorSetLayoutCreateInfo.setBindings(descriptorSetLayoutBingings);

    _descriptorSetLayout = vk::raii::DescriptorSetLayout(*_device, descriptorSetLayoutCreateInfo);

    vk::PushConstantRange pushConstantRange;
    pushConstantRange.setOffset(0);
    pushConstantRange.setSize(sizeof(ShaderStructures::PushConstants));
    pushConstantRange.setStageFlags(vk::ShaderStageFlagBits::eVertex);

    std::vector<vk::DescriptorSetLayout> descriptorSetLayouts = { *globalDescriptorSetLayout, *_descriptorSetLayout };

    vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo;
    pipelineLayoutCreateInfo.setPushConstantRanges(pushConstantRange);
    pipelineLayoutCreateInfo.setSetLayouts(descriptorSetLayouts);

    _pipelineLayout = vk::raii::PipelineLayout(*_device, pipelineLayoutCreateInfo);

    _pipeline = VulkanUtilities::createPipeline(*device, _pipelineLayout, renderPass, _vertexShader.getModule(), _fragmentShader.getModule());
}

GLTFPBRMaterial::GLTFPBRMaterial(GLTFPBRMaterial&& other) noexcept
{
    this->_device = other._device;
    this->_memoryAllocator = std::move(other._memoryAllocator);
    this->_immediateSubmit = other._immediateSubmit;
    this->_descriptorAllocator = other._descriptorAllocator;

    this->_pipeline = std::move(other._pipeline);
    this->_pipelineLayout = std::move(other._pipelineLayout);
    this->_descriptorSetLayout = std::move(other._descriptorSetLayout);

    this->_vertexShader = std::move(other._vertexShader);
    this->_fragmentShader = std::move(other._fragmentShader);

    this->_resources = std::move(other._resources);

    other._device = nullptr;
    other._immediateSubmit = nullptr;
    other._descriptorAllocator = nullptr;
}

GLTFPBRMaterial& GLTFPBRMaterial::operator=(GLTFPBRMaterial&& other) noexcept
{
    if (this != &other)
    {
        this->_device = other._device;
        this->_memoryAllocator = std::move(other._memoryAllocator);
        this->_immediateSubmit = other._immediateSubmit;
        this->_descriptorAllocator = other._descriptorAllocator;

        this->_pipeline = std::move(other._pipeline);
        this->_pipelineLayout = std::move(other._pipelineLayout);
        this->_descriptorSetLayout = std::move(other._descriptorSetLayout);

        this->_vertexShader = std::move(other._vertexShader);
        this->_fragmentShader = std::move(other._fragmentShader);

        this->_resources = std::move(other._resources);

        other._device = nullptr;
        other._immediateSubmit = nullptr;
        other._descriptorAllocator = nullptr;
    }
    
    return *this;
}

std::shared_ptr<vk::raii::DescriptorSet> GLTFPBRMaterial::createDescriptorSet(std::shared_ptr<Material> material)
{
    return std::make_shared<vk::raii::DescriptorSet>(nullptr);
}

const vk::raii::Pipeline* SVMV::GLTFPBRMaterial::getPipeline() const
{
    return &_pipeline;
}

const vk::raii::PipelineLayout* SVMV::GLTFPBRMaterial::getPipelineLayout() const
{
    return &_pipelineLayout;
}

//std::shared_ptr<MaterialInstance> GLTFPBRMaterial::generateMaterialInstance(std::shared_ptr<Material> material)
//{
//    std::shared_ptr<MaterialInstance> instance = std::make_shared<MaterialInstance>();
//    instance->materialName = MaterialName::GLTFPBR;
//
//    MaterialResources resources = { .colorSampler = vk::raii::Sampler(nullptr) };
//
//    // get descriptor set
//    instance->descriptorSet = _descriptorAllocator->allocateSet(_descriptorSetLayout);
//
//    // create buffer for uniform data and image for texture
//    auto bufferUsage = vk::BufferUsageFlagBits::eUniformBuffer;
//    resources.buffer = std::make_shared<VulkanBuffer>();
//    resources.buffer->create(_device, _memoryAllocator, sizeof(MaterialUniformParameters), bufferUsage, VMA_MEMORY_USAGE_CPU_TO_GPU);
//
//    void* mappedData = nullptr;
//    vmaMapMemory(_memoryAllocator, resources.buffer->allocation, &mappedData);
//    MaterialUniformParameters* data = reinterpret_cast<MaterialUniformParameters*>(mappedData);
//
//    data->baseColorFactor = material->baseColorFactor;
//    data->roughnessMetallicFactors[0] = material->metallicFactor;
//    data->roughnessMetallicFactors[1] = material->roughnessFactor;
//
//    auto imageUsage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst;
//    resources.colorImage = std::make_shared<VulkanImage>();
//    resources.colorImage->create(_device, _memoryAllocator, material->diffuseTexture->width, material->diffuseTexture->height, vk::Format::eR8G8B8A8Srgb, imageUsage, _immediateSubmit);
//    resources.colorImage->copyDataToImage(material->diffuseTexture->data.data(), material->diffuseTexture->data.size());
//
//    vk::SamplerCreateInfo samplerCreateInfo = {};
//    samplerCreateInfo.sType = vk::StructureType::eSamplerCreateInfo;
//    samplerCreateInfo.magFilter = vk::Filter::eLinear;
//    samplerCreateInfo.minFilter = vk::Filter::eLinear;
//
//    resources.colorSampler = (*_device).createSampler(samplerCreateInfo);
//
//    // write to the descriptor set
//    vk::DescriptorBufferInfo bufferInfo = {};
//    bufferInfo.buffer = resources.buffer->buffer;
//    bufferInfo.offset = 0;
//    bufferInfo.range = sizeof(MaterialUniformParameters);
//
//    vk::DescriptorImageInfo imageInfo = {};
//    imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
//    imageInfo.imageView = resources.colorImage->imageView;
//    imageInfo.sampler = resources.colorSampler;
//
//    vk::WriteDescriptorSet writes[2] = {};
//
//    writes[0].dstBinding = 0;
//    writes[0].dstSet = instance->descriptorSet;
//    writes[0].descriptorCount = 1;
//    writes[0].descriptorType = vk::DescriptorType::eUniformBuffer;
//    writes[0].pBufferInfo = &bufferInfo;
//
//    writes[1].dstBinding = 1;
//    writes[1].dstSet = instance->descriptorSet;
//    writes[1].descriptorCount = 1;
//    writes[1].descriptorType = vk::DescriptorType::eCombinedImageSampler;
//    writes[1].pImageInfo = &imageInfo;
//
//    (*_device).updateDescriptorSets(writes, 0);
//
//    // store the instance in this, and return a pointer to it
//
//    _resources.push_back(resources);
//
//    vmaUnmapMemory(_memoryAllocator, resources.buffer->allocation);
//    
//    return instance;
//    return nullptr;
//}
