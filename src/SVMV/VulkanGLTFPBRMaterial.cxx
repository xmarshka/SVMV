#include <SVMV/VulkanGLTFPBRMaterial.hxx>
#include <vk_mem_alloc.h>

using namespace SVMV;

void GLTFPBRMaterial::initialize(vk::Device device, VmaAllocator allocator, vk::RenderPass renderPass, vkb::Swapchain swapchain)
{
    _device = device;
    _memoryAllocator = allocator;

    generatePipeline(device, renderPass, swapchain);
}

void GLTFPBRMaterial::free()
{
    _device.destroyPipelineLayout(_pipelineLayout);
    _device.destroyPipeline(_pipeline);

    _device.destroyDescriptorSetLayout(_layout);

    for (const auto& resource : _resources)
    {
        resource.buffer->free();
        resource.colorImage->free();
        _device.destroySampler(resource.colorSampler);
    }
}

void GLTFPBRMaterial::generatePipeline(vk::Device device, vk::RenderPass renderPass, vkb::Swapchain swapchain)
{
    VulkanShader vertexShader(device, VulkanShader::ShaderType::VERTEX, "shader.vert");
    VulkanShader fragmentShader(device, VulkanShader::ShaderType::FRAGMENT, "shader.frag");

    vk::PipelineShaderStageCreateInfo shaderStages[2];

    shaderStages[0].sType = vk::StructureType::ePipelineShaderStageCreateInfo;
    shaderStages[0].stage = vk::ShaderStageFlagBits::eVertex;
    shaderStages[0].module = vertexShader.shader;
    shaderStages[0].pName = "main";

    shaderStages[1].sType = vk::StructureType::ePipelineShaderStageCreateInfo;
    shaderStages[1].stage = vk::ShaderStageFlagBits::eFragment;
    shaderStages[1].module = fragmentShader.shader;
    shaderStages[1].pName = "main";

    vk::PipelineVertexInputStateCreateInfo vertexInputStateInfo = {};
    vertexInputStateInfo.sType = vk::StructureType::ePipelineVertexInputStateCreateInfo;
    vertexInputStateInfo.vertexBindingDescriptionCount = 0;
    vertexInputStateInfo.vertexAttributeDescriptionCount = 0;

    vk::PipelineInputAssemblyStateCreateInfo assemblyStateInfo = {};
    assemblyStateInfo.sType = vk::StructureType::ePipelineInputAssemblyStateCreateInfo;
    assemblyStateInfo.topology = vk::PrimitiveTopology::eTriangleList;
    assemblyStateInfo.primitiveRestartEnable = vk::False;

    vk::Viewport viewport;
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = swapchain.extent.width;
    viewport.height = swapchain.extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    vk::Rect2D scissor;
    scissor.offset = vk::Offset2D(0, 0);
    scissor.extent = vk::Extent2D(swapchain.extent.width, swapchain.extent.height);

    std::vector<vk::DynamicState> dynamicStates = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };
    vk::PipelineDynamicStateCreateInfo dynamicStateInfo = {};
    dynamicStateInfo.sType = vk::StructureType::ePipelineDynamicStateCreateInfo;
    dynamicStateInfo.dynamicStateCount = dynamicStates.size();
    dynamicStateInfo.pDynamicStates = dynamicStates.data();

    vk::PipelineViewportStateCreateInfo viewportStateInfo = {};
    viewportStateInfo.sType = vk::StructureType::ePipelineViewportStateCreateInfo;
    viewportStateInfo.viewportCount = 1;
    viewportStateInfo.pViewports = &viewport;
    viewportStateInfo.scissorCount = 1;
    viewportStateInfo.pScissors = &scissor;

    vk::PipelineRasterizationStateCreateInfo rasterizationStateInfo = {};
    rasterizationStateInfo.sType = vk::StructureType::ePipelineRasterizationStateCreateInfo;
    rasterizationStateInfo.depthClampEnable = vk::False;
    rasterizationStateInfo.rasterizerDiscardEnable = vk::False;
    rasterizationStateInfo.polygonMode = vk::PolygonMode::eFill;
    rasterizationStateInfo.lineWidth = 1.0f;
    rasterizationStateInfo.cullMode = vk::CullModeFlagBits::eBack;
    rasterizationStateInfo.frontFace = vk::FrontFace::eCounterClockwise;
    rasterizationStateInfo.depthBiasEnable = vk::False;

    vk::PipelineMultisampleStateCreateInfo multisamplingInfo = {};
    multisamplingInfo.sType = vk::StructureType::ePipelineMultisampleStateCreateInfo;
    multisamplingInfo.sampleShadingEnable = vk::False;
    multisamplingInfo.rasterizationSamples = vk::SampleCountFlagBits::e1;

    vk::PipelineColorBlendAttachmentState colorBlendAttachmentState = {};
    colorBlendAttachmentState.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    colorBlendAttachmentState.blendEnable = vk::False;

    vk::PipelineColorBlendStateCreateInfo colorBlendStateInfo = {};
    colorBlendStateInfo.sType = vk::StructureType::ePipelineColorBlendStateCreateInfo;
    colorBlendStateInfo.logicOpEnable = vk::False;
    colorBlendStateInfo.attachmentCount = 1;
    colorBlendStateInfo.pAttachments = &colorBlendAttachmentState;

    vk::PushConstantRange pushConstantRange = {};
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(ShaderStructures::PushConstants);
    pushConstantRange.stageFlags = vk::ShaderStageFlagBits::eVertex;

    vk::DescriptorSetLayoutBinding bindings[2];
    bindings[0].binding = 0;
    bindings[0].descriptorCount = 1;
    bindings[0].descriptorType = vk::DescriptorType::eUniformBuffer;

    bindings[1].binding = 1;
    bindings[1].descriptorCount = 1;
    bindings[1].descriptorType = vk::DescriptorType::eCombinedImageSampler;

    vk::DescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = vk::StructureType::eDescriptorSetLayoutCreateInfo;
    layoutInfo.bindingCount = 2;
    layoutInfo.pBindings = bindings;

    _layout = device.createDescriptorSetLayout(layoutInfo);

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = vk::StructureType::ePipelineLayoutCreateInfo;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &_layout;

    _pipelineLayout = device.createPipelineLayout(pipelineLayoutInfo);

    vk::GraphicsPipelineCreateInfo info = {};
    info.sType = vk::StructureType::eGraphicsPipelineCreateInfo;
    info.stageCount = 2;
    info.pStages = shaderStages;
    info.pVertexInputState = &vertexInputStateInfo;
    info.pInputAssemblyState = &assemblyStateInfo;
    info.pViewportState = &viewportStateInfo;
    info.pRasterizationState = &rasterizationStateInfo;
    info.pMultisampleState = &multisamplingInfo;
    info.pColorBlendState = &colorBlendStateInfo;
    info.pDynamicState = &dynamicStateInfo;
    info.layout = _pipelineLayout;
    info.renderPass = renderPass;
    info.subpass = 0;

    auto result = device.createGraphicsPipeline(nullptr, info);

    if (result.result != vk::Result::eSuccess)
    {
        throw std::runtime_error("vulkan: failed to create graphics pipeline");
    }

    _pipeline = result.value;
}

std::shared_ptr<MaterialInstance> GLTFPBRMaterial::generateMaterialInstance(std::shared_ptr<Material> material, VulkanDescriptorAllocator& descriptorAllocator)
{
    std::shared_ptr<MaterialInstance> instance = std::make_shared<MaterialInstance>();
    instance->materialName = MaterialName::GLTFPBR;

    MaterialResources resources;

    // get descriptor set form some pool
    instance->descriptorSet = descriptorAllocator.allocateSet(_layout);

    // create buffer for uniform data and image for texture
    auto bufferUsage = vk::BufferUsageFlagBits::eUniformBuffer;
    resources.buffer = std::make_shared<VulkanBuffer>();
    resources.buffer->create(_memoryAllocator, sizeof(MaterialUniformParameters), bufferUsage, VMA_MEMORY_USAGE_CPU_TO_GPU);

    void* mappedData = nullptr;
    vmaMapMemory(_memoryAllocator, resources.buffer->allocation, &mappedData);
    MaterialUniformParameters* data = reinterpret_cast<MaterialUniformParameters*>(mappedData);

    data->baseColorFactor = material->baseColorFactor;
    data->roughnessMetallicFactors[0] = material->metallicFactor;
    data->roughnessMetallicFactors[1] = material->roughnessFactor;

    auto imageUsage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst;
    resources.colorImage = std::make_shared<VulkanImage>();
    resources.colorImage->create(_device, _memoryAllocator, material->diffuseTexture->width, material->diffuseTexture->height, vk::Format::eR8G8B8A8Srgb, imageUsage, _immediateSubmit);
    resources.colorImage->copyDataToImage(material->diffuseTexture->data.data(), material->diffuseTexture->data.size());

    vk::SamplerCreateInfo samplerCreateInfo = {};
    samplerCreateInfo.sType = vk::StructureType::eSamplerCreateInfo;
    samplerCreateInfo.magFilter = vk::Filter::eLinear;
    samplerCreateInfo.minFilter = vk::Filter::eLinear;

    resources.colorSampler = _device.createSampler(samplerCreateInfo);

    // write to the descriptor set
    vk::DescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer = resources.buffer->buffer;
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(MaterialUniformParameters);

    vk::DescriptorImageInfo imageInfo = {};
    imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    imageInfo.imageView = resources.colorImage->imageView;
    imageInfo.sampler = resources.colorSampler;

    vk::WriteDescriptorSet writes[2] = {};

    writes[0].dstBinding = 0;
    writes[0].dstSet = instance->descriptorSet;
    writes[0].descriptorCount = 1;
    writes[0].descriptorType = vk::DescriptorType::eUniformBuffer;
    writes[0].pBufferInfo = &bufferInfo;

    writes[1].dstBinding = 1;
    writes[1].dstSet = instance->descriptorSet;
    writes[1].descriptorCount = 1;
    writes[1].descriptorType = vk::DescriptorType::eCombinedImageSampler;
    writes[1].pImageInfo = &imageInfo;

    _device.updateDescriptorSets(writes, 0);

    // store the instance in this, and return a pointer to it

    _resources.push_back(resources);
    
    return instance;
}

MaterialPipeline SVMV::GLTFPBRMaterial::getMaterialPipeline()
{
    MaterialPipeline pipeline;
    pipeline.layout = _pipelineLayout;
    pipeline.pipeline = _pipeline;

    return pipeline;
}
