#include <SVMV/VulkanUtilities.hxx>

using namespace SVMV;

VulkanUtilities::ImmediateSubmit::ImmediateSubmit(vk::raii::Device* device, vk::raii::Queue* queue, unsigned queueFamily)
{
    _device = device;
    _queue = queue;

    vk::CommandPoolCreateInfo commandPoolCreateInfo;
    commandPoolCreateInfo.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
    commandPoolCreateInfo.setQueueFamilyIndex(queueFamily);

    _commandPool = vk::raii::CommandPool(*_device, commandPoolCreateInfo);

    vk::FenceCreateInfo fenceCreateInfo;
    fenceCreateInfo.setFlags(vk::FenceCreateFlagBits::eSignaled);

    _fence = vk::raii::Fence(*_device, fenceCreateInfo);

    vk::CommandBufferAllocateInfo commandBufferAllocateInfo;
    commandBufferAllocateInfo.setLevel(vk::CommandBufferLevel::ePrimary);
    commandBufferAllocateInfo.setCommandPool(_commandPool);
    commandBufferAllocateInfo.setCommandBufferCount(1);

    _commandBuffer = std::move(vk::raii::CommandBuffers(*_device, commandBufferAllocateInfo)[0]);
}

VulkanUtilities::ImmediateSubmit::ImmediateSubmit(ImmediateSubmit&& other) noexcept
{
    this->_device = other._device;
    this->_queue = other._queue;

    this->_commandPool = std::move(other._commandPool);
    this->_commandBuffer = std::move(other._commandBuffer);
    this->_fence = std::move(other._fence);
}

VulkanUtilities::ImmediateSubmit& VulkanUtilities::ImmediateSubmit::operator=(ImmediateSubmit&& other) noexcept
{
    if (this != &other)
    {
        this->_device = other._device;
        this->_queue = other._queue;

        this->_commandPool = std::move(other._commandPool);
        this->_commandBuffer = std::move(other._commandBuffer);
        this->_fence = std::move(other._fence);
    }

    return *this;
}

void VulkanUtilities::ImmediateSubmit::submit(std::function<void(const vk::raii::CommandBuffer& commandBuffer)>&& lambda)
{
    vk::Result waitForFencesResult = _device->waitForFences(*(_fence), true, INT16_MAX);

    if (waitForFencesResult != vk::Result::eSuccess)
    {
        throw std::runtime_error("vulkan: failure waiting for fences");
    }

    _device->resetFences(*_fence);
    _commandBuffer.reset();

    vk::CommandBufferBeginInfo commandBufferBeginInfo = {};

    _commandBuffer.begin(commandBufferBeginInfo);

    lambda(_commandBuffer);

    _commandBuffer.end();

    vk::SubmitInfo submitInfo;
    submitInfo.setCommandBuffers(*(_commandBuffer));

    _queue->submit(submitInfo, _fence);
}

VulkanUtilities::DescriptorAllocator::DescriptorAllocator(vk::raii::Device* device)
    : _device(device)
{}

VulkanUtilities::DescriptorAllocator::DescriptorAllocator(DescriptorAllocator&& other) noexcept
{
    this->_device = other._device;
    this->_availablePools = other._availablePools;
    this->_filledPools = other._filledPools;

    other._device = nullptr;
    other._availablePools.clear();
    other._filledPools.clear();
}

VulkanUtilities::DescriptorAllocator& VulkanUtilities::DescriptorAllocator::operator=(DescriptorAllocator&& other) noexcept
{
    if (this != &other)
    {
        this->destroyPools();

        this->_device = other._device;
        this->_availablePools = other._availablePools;
        this->_filledPools = other._filledPools;

        other._device = nullptr;
        other._availablePools.clear();
        other._filledPools.clear();
    }

    return *this;
}

void VulkanUtilities::DescriptorAllocator::destroyPools()
{
    _availablePools.clear();
    _filledPools.clear();
}

void VulkanUtilities::DescriptorAllocator::clearPools()
{
    for (const auto& pool : _availablePools)
    {
        pool->reset();
    }

    for (const auto& pool : _filledPools)
    {
        pool->reset();
        _availablePools.push_back(pool);
    }

    _filledPools.clear();
}

vk::raii::DescriptorSet VulkanUtilities::DescriptorAllocator::allocateSet(const vk::raii::DescriptorSetLayout& layout)
{
    std::shared_ptr<vk::raii::DescriptorPool> pool = getPool();

    vk::DescriptorSetAllocateInfo info = {};
    info.setDescriptorPool(*pool);
    info.setDescriptorSetCount(1);
    info.setSetLayouts(*(layout));

    vk::raii::DescriptorSets sets(nullptr);

    try
    {
        sets = vk::raii::DescriptorSets(*_device, info);
    }
    catch (const vk::SystemError& e)
    {
        if (e.code() == vk::make_error_code(vk::Result::eErrorOutOfPoolMemory) || e.code() == vk::make_error_code(vk::Result::eErrorFragmentedPool))
        {
            _filledPools.push_back(pool);

            pool = getPool();
            info.descriptorPool = *pool;

            sets = vk::raii::DescriptorSets(*_device, info);
        }
        else
        {
            throw std::runtime_error("DescriptorAllocator: failed to allocate descriptor set");
        }
    }

    _availablePools.push_back(pool);

    return std::move(sets[0]);
}

std::shared_ptr<vk::raii::DescriptorPool> VulkanUtilities::DescriptorAllocator::getPool()
{
    std::shared_ptr<vk::raii::DescriptorPool> pool;

    if (_availablePools.size() != 0)
    {
        pool = _availablePools.back();
        _availablePools.pop_back();
    }
    else
    {
        pool = createPool();
        _availablePools.push_back(pool);
    }

    return pool;
}

std::shared_ptr<vk::raii::DescriptorPool> VulkanUtilities::DescriptorAllocator::createPool()
{
    vk::DescriptorPoolSize poolSizes[2];
    poolSizes[0].setType(vk::DescriptorType::eUniformBuffer);
    poolSizes[0].setDescriptorCount(_setsPerPool);

    poolSizes[1].setType(vk::DescriptorType::eCombinedImageSampler);
    poolSizes[1].setDescriptorCount(_setsPerPool);

    vk::DescriptorPoolCreateInfo info;
    info.setMaxSets(_setsPerPool);
    info.setPoolSizes(poolSizes);
    info.setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet);

    std::shared_ptr<vk::raii::DescriptorPool> pool = std::make_shared<vk::raii::DescriptorPool>(*_device, info);

    if (_setsPerPool < 4096)
    {
        _setsPerPool *= 2;
    }

    return pool;
}

VulkanUtilities::VmaAllocatorWrapper::VmaAllocatorWrapper(const vk::raii::Instance& instance, const vk::raii::PhysicalDevice& physicalDevice, const vk::raii::Device& device)
{
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.instance = *instance;
    allocatorInfo.physicalDevice = *physicalDevice;
    allocatorInfo.device = *device;
    allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

    VkResult result = vmaCreateAllocator(&allocatorInfo, &_allocator);

    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("vma: failed to create allocator");
    }
}

VulkanUtilities::VmaAllocatorWrapper::VmaAllocatorWrapper(VmaAllocatorWrapper&& other) noexcept
{
    this->_allocator = other._allocator;

    other._allocator = nullptr;
}

VulkanUtilities::VmaAllocatorWrapper& VulkanUtilities::VmaAllocatorWrapper::operator=(VmaAllocatorWrapper&& other) noexcept
{
    if (this != &other)
    {
        if (_allocator != nullptr)
        {
            vmaDestroyAllocator(_allocator);
        }

        this->_allocator = other._allocator;

        other._allocator = nullptr;
    }

    return *this;
}

VulkanUtilities::VmaAllocatorWrapper::~VmaAllocatorWrapper()
{
    if (_allocator != nullptr)
    {
        vmaDestroyAllocator(_allocator);
    }
}

VmaAllocator VulkanUtilities::VmaAllocatorWrapper::getAllocator() const noexcept
{
    return _allocator;
}

vk::raii::Pipeline VulkanUtilities::createPipeline(const vk::raii::Device& device, const vk::raii::PipelineLayout& pipelineLayout, const vk::raii::RenderPass& renderPass, const vk::raii::ShaderModule& vertexShader, const vk::raii::ShaderModule& fragmentShader)
{
    vk::PipelineShaderStageCreateInfo shaderStages[2];
    shaderStages[0].setStage(vk::ShaderStageFlagBits::eVertex);
    shaderStages[0].setModule(vertexShader);
    shaderStages[0].setPName("main");

    shaderStages[1].setStage(vk::ShaderStageFlagBits::eFragment);
    shaderStages[1].setModule(fragmentShader);
    shaderStages[1].setPName("main");

    vk::PipelineVertexInputStateCreateInfo vertexInputStateInfo;
    vertexInputStateInfo.setVertexBindingDescriptionCount(0);
    vertexInputStateInfo.setVertexAttributeDescriptionCount(0);

    vk::PipelineInputAssemblyStateCreateInfo assemblyStateInfo;
    assemblyStateInfo.setTopology(vk::PrimitiveTopology::eTriangleList);
    assemblyStateInfo.setPrimitiveRestartEnable(vk::False);

    vk::Viewport viewport;
    viewport.setX(0.0f);
    viewport.setY(0.0f);
    viewport.setWidth(640);
    viewport.setHeight(480);
    viewport.setMinDepth(0.0f);
    viewport.setMaxDepth(1.0f);

    vk::Rect2D scissor;
    scissor.setOffset(vk::Offset2D(0, 0));
    scissor.setExtent(vk::Extent2D(640, 480));

    std::vector<vk::DynamicState> dynamicStates = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };
    vk::PipelineDynamicStateCreateInfo dynamicStateInfo;
    dynamicStateInfo.setDynamicStates(dynamicStates);

    vk::PipelineViewportStateCreateInfo viewportStateInfo;
    viewportStateInfo.setViewports(viewport);
    viewportStateInfo.setScissors(scissor);

    vk::PipelineRasterizationStateCreateInfo rasterizationStateInfo;
    rasterizationStateInfo.setDepthClampEnable(vk::False);
    rasterizationStateInfo.setRasterizerDiscardEnable(vk::False);
    rasterizationStateInfo.setPolygonMode(vk::PolygonMode::eFill);
    rasterizationStateInfo.setLineWidth(1.0f);
    rasterizationStateInfo.setCullMode(vk::CullModeFlagBits::eBack);
    rasterizationStateInfo.setFrontFace(vk::FrontFace::eCounterClockwise);
    rasterizationStateInfo.setDepthBiasEnable(vk::False);

    vk::PipelineMultisampleStateCreateInfo multisamplingInfo;
    multisamplingInfo.setSampleShadingEnable(vk::False);
    multisamplingInfo.setRasterizationSamples(vk::SampleCountFlagBits::e1);

    vk::PipelineColorBlendAttachmentState colorBlendAttachmentState;
    colorBlendAttachmentState.setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);
    colorBlendAttachmentState.setBlendEnable(vk::False);

    vk::PipelineColorBlendStateCreateInfo colorBlendStateInfo;
    colorBlendStateInfo.setLogicOpEnable(vk::False);
    colorBlendStateInfo.setAttachments(colorBlendAttachmentState);

    vk::GraphicsPipelineCreateInfo pipelineInfo;
    pipelineInfo.setStages(shaderStages);
    pipelineInfo.setPVertexInputState(&vertexInputStateInfo);
    pipelineInfo.setPInputAssemblyState(&assemblyStateInfo);
    pipelineInfo.setPViewportState(&viewportStateInfo);
    pipelineInfo.setPRasterizationState(&rasterizationStateInfo);
    pipelineInfo.setPMultisampleState(&multisamplingInfo);
    pipelineInfo.setPColorBlendState(&colorBlendStateInfo);
    pipelineInfo.setPDynamicState(&dynamicStateInfo);
    pipelineInfo.setLayout(pipelineLayout);
    pipelineInfo.setRenderPass(renderPass);
    pipelineInfo.setSubpass(0);

    return vk::raii::Pipeline(device, nullptr, pipelineInfo);
}
