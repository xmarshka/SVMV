#include <SVMV/VulkanRenderer.hxx>

using namespace SVMV;

VulkanRenderer::VulkanRenderer() : _framesInFlight(2), _graphicsQueueFamily(0), _presentQueueFamily(0), _activeFrame(0), resized(false)
{
    _imageReadySemaphores.resize(_framesInFlight);
    _renderCompleteSemaphores.resize(_framesInFlight);
    _inFlightFences.resize(_framesInFlight);
}

VulkanRenderer::VulkanRenderer(unsigned framesInFlight) : _framesInFlight(framesInFlight), _graphicsQueueFamily(0), _presentQueueFamily(0), _activeFrame(0), resized(false)
{
    _imageReadySemaphores.resize(framesInFlight);
    _renderCompleteSemaphores.resize(framesInFlight);
    _inFlightFences.resize(framesInFlight);
}

void VulkanRenderer::createInstance()
{
    vkb::InstanceBuilder builder;

    builder.set_app_name("SVMV");
    builder.set_engine_name("SVMV");
    builder.request_validation_layers();
    builder.use_default_debug_messenger();

    builder.desire_api_version(1, 3);

    vkb::Result<vkb::Instance> result = builder.build();

    if (!result)
    {
        throw std::runtime_error("bk-bootstrap: failed to build");
    }

    _bootstrapInstance = result.value();
    _instance = _bootstrapInstance.instance;

    _messenger = _bootstrapInstance.debug_messenger;
}

void VulkanRenderer::initializeRenderer(vk::SurfaceKHR surface)
{
    setSurface(surface);
    createDevice();
    getQueues();
    createSwapchain();

    createRenderPass();
    //createGraphicsPipeline();
    createFramebuffers();
    createCommandPool();
    //createVertexBuffer();
    //createCommandBuffers();
    createSynchronisationObjects();
}

void VulkanRenderer::loadScene(std::shared_ptr<Scene> scene)
{
    _scene = std::make_shared<VulkanScene>(_physicalDevice, _instance, _device, _commandPool, _graphicsQueue);
    _scene->setScene(scene, _renderPass, _bootstrapSwapchain, _framesInFlight);
}

void VulkanRenderer::draw()
{
    _device.waitForFences(1, &_inFlightFences[_activeFrame], vk::True, UINT64_MAX);

    auto result = _device.acquireNextImageKHR(_swapchain, UINT64_MAX, _imageReadySemaphores[_activeFrame]);

    if (result.result == vk::Result::eErrorOutOfDateKHR || resized)
    {
        resized = false;
        recreateSwapchain();
        return;
    }
    else if (result.result != vk::Result::eSuccess)
    {
        throw std::runtime_error("vulkan: failed to acquire next image");
    }

    _device.resetFences(1, &_inFlightFences[_activeFrame]);

    unsigned imageIndex = result.value;

    //_commandBuffers[_activeFrame].reset();
    //recordCommandBuffer(_commandBuffers[_activeFrame], imageIndex);

    auto vector = _scene->recordFrameCommandBuffers(_activeFrame, _framebuffers[imageIndex], _bootstrapSwapchain.extent.width, _bootstrapSwapchain.extent.height);

    vk::SubmitInfo submitInfo = {};
    submitInfo.sType = vk::StructureType::eSubmitInfo;

    vk::Semaphore waitSemaphores[] = { _imageReadySemaphores[_activeFrame] };
    vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput }; // TODO

    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    //submitInfo.commandBufferCount = 1;
    //submitInfo.pCommandBuffers = &_commandBuffers[_activeFrame];

    submitInfo.commandBufferCount = vector.size();
    submitInfo.pCommandBuffers = vector.data();

    vk::Semaphore signalSemaphores[] = { _renderCompleteSemaphores[_activeFrame] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (_graphicsQueue.submit(1, &submitInfo, _inFlightFences[_activeFrame]) != vk::Result::eSuccess)
    {
        throw std::runtime_error("vulkan: failed to submit to graphics queue");
    }

    vk::PresentInfoKHR presentInfo = {};
    presentInfo.sType = vk::StructureType::ePresentInfoKHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &_swapchain;
    presentInfo.pImageIndices = &imageIndex;

    vk::Result presentResult = _presentQueue.presentKHR(presentInfo);

    if (presentResult == vk::Result::eErrorOutOfDateKHR || /*presentResult != vk::Result::eSuboptimalKHR || */resized)
    {
        resized = false;
        recreateSwapchain();
    }
    else if (presentResult != vk::Result::eSuccess)
    {
        throw std::runtime_error("vulkan: failed to present image");
    }

    _activeFrame = (_activeFrame + 1) % _framesInFlight;
}

void VulkanRenderer::cleanup()
{
    _scene.reset();

    _device.destroyBuffer(_vertexBuffer);
    _device.freeMemory(_vertexBufferMemory);

    for (int i = 0; i < _framesInFlight; i++)
    {
        _device.destroySemaphore(_imageReadySemaphores[i]);
        _device.destroySemaphore(_renderCompleteSemaphores[i]);
        _device.destroyFence(_inFlightFences[i]);
    }

    _device.destroyCommandPool(_commandPool);

    for (int i = 0; i < _framebuffers.size(); i++)
    {
        _device.destroyFramebuffer(_framebuffers[i]);
    }

    _device.destroyPipeline(_pipeline);
    _device.destroyPipelineLayout(_pipelineLayout);
    _device.destroyRenderPass(_renderPass);

    for (int i = 0; i < _imageViews.size(); i++)
    {
        _device.destroyImageView(_imageViews[i]);
    }

    vkb::destroy_swapchain(_bootstrapSwapchain);
    vkb::destroy_device(_bootstrapDevice);
    vkb::destroy_surface(_bootstrapInstance, _surface);
    vkb::destroy_instance(_bootstrapInstance); // this also destroys the debug messenger
}

vk::Instance VulkanRenderer::getInstance()
{
    return _instance;
}

vk::Device VulkanRenderer::getDevice()
{
    return _device;
}

std::string VulkanRenderer::readFile(const std::string& file)
{
    std::stringstream output;
    std::string line;

    std::string test = RESOURCE_DIR"/shaders/" + file;

    std::ifstream fileStream(test);

    if (!fileStream.is_open())
    {
        throw std::runtime_error("ifstream: failed to open file: " + file);
    }

    while (std::getline(fileStream, line))
    {
        output << line;
        output << '\n';
    }

    return output.str();
}

std::string VulkanRenderer::preprocessShader(const std::string& name, const std::string& shaderCode, shaderc_shader_kind shaderKind)
{
    shaderc::CompileOptions options;

    shaderc::PreprocessedSourceCompilationResult result = _shaderCompiler.PreprocessGlsl(shaderCode, shaderKind, name.c_str(), options);

    if (result.GetCompilationStatus() != shaderc_compilation_status_success)
    {
        throw std::runtime_error("shaderc: failed to preprocess shader: " + result.GetErrorMessage());
    }

    return std::string(result.cbegin(), result.cend());
}

std::vector<uint32_t> VulkanRenderer::compileShader(const std::string& name, const std::string& shaderCode, shaderc_shader_kind shaderKind, bool optimize)
{
    shaderc::CompileOptions options;

    options.SetSourceLanguage(shaderc_source_language_glsl);

    shaderc::SpvCompilationResult result = _shaderCompiler.CompileGlslToSpv(shaderCode, shaderKind, name.c_str());

    if (result.GetCompilationStatus() != shaderc_compilation_status_success)
    {
        throw std::runtime_error("shaderc: failed to compile shader: " + result.GetErrorMessage());
    }

    return std::vector<uint32_t>(result.cbegin(), result.cend());
}

uint32_t VulkanRenderer::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags propertyFlags)
{
    vk::PhysicalDeviceMemoryProperties properties = _physicalDevice.getMemoryProperties();

    for (unsigned i = 0; i < properties.memoryTypeCount; i++)
    {
        if (typeFilter & (1 << i) && (properties.memoryTypes[i].propertyFlags & propertyFlags) == propertyFlags)
        {
            return i;
        }
    }

    throw std::runtime_error("vulkan: failed to find suitable memory type");
    return 0;
}

void VulkanRenderer::setSurface(vk::SurfaceKHR surface)
{
    _surface = surface;
}

void VulkanRenderer::createDevice()
{
    if (!_instance || !_surface)
    {
        return;
    }

    vkb::PhysicalDeviceSelector selector(_bootstrapInstance, _surface);

    // TODO: selector.set_required_extensions();
    selector.add_required_extension(vk::KHRBufferDeviceAddressExtensionName);
    
    vk::PhysicalDeviceVulkan12Features features = {};
    features.sType = vk::StructureType::ePhysicalDeviceVulkan12Features;
    features.bufferDeviceAddress = true;

    selector.set_required_features_12(features);

    vkb::Result<vkb::PhysicalDevice> physicalDeviceResult = selector.select();

    if (!physicalDeviceResult)
    {
        throw std::runtime_error("bk-bootstrap: failed to select physical device");
    }

    vkb::PhysicalDevice physicalDevice = physicalDeviceResult.value();
    _physicalDevice = physicalDevice.physical_device;

    vkb::DeviceBuilder builder(physicalDevice);

    vkb::Result<vkb::Device> deviceResult = builder.build();

    if (!deviceResult)
    {
        throw std::runtime_error("bk-bootstrap: failed to create device");
    }

    _bootstrapDevice = deviceResult.value();
    _device = _bootstrapDevice.device;
}

void VulkanRenderer::getQueues()
{
    _graphicsQueue = _bootstrapDevice.get_queue(vkb::QueueType::graphics).value();
    _graphicsQueueFamily = _bootstrapDevice.get_queue_index(vkb::QueueType::graphics).value();

    _presentQueue = _bootstrapDevice.get_queue(vkb::QueueType::present).value();
    _presentQueueFamily = _bootstrapDevice.get_queue_index(vkb::QueueType::present).value();
}

void VulkanRenderer::createSwapchain()
{
    vkb::SwapchainBuilder builder(_bootstrapDevice);

    /* "By default, the swapchain will use the VK_FORMAT_B8G8R8A8_SRGB or VK_FORMAT_R8G8B8A8_SRGB image format
    with the color space VK_COLOR_SPACE_SRGB_NONLINEAR_KHR.The present mode will default to VK_PRESENT_MODE_MAILBOX_KHR
    if available and fallback to VK_PRESENT_MODE_FIFO_KHR.The image usage default flag is VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT."
    (via https://github.com/charles-lunarg/vk-bootstrap/blob/main/docs/getting_started.md) */

    vkb::Result<vkb::Swapchain> result = builder.build();

    if (!result)
    {
        throw std::runtime_error("vk-bootstrap: failed to create swapchain");
    }

    _bootstrapSwapchain = result.value();
    _swapchain = _bootstrapSwapchain.swapchain;

    _images.resize(_bootstrapSwapchain.get_images().value().size());
    _imageViews.resize(_bootstrapSwapchain.get_images().value().size());

    std::vector<VkImageView> views = _bootstrapSwapchain.get_image_views().value(); // get_image_views apparently creates the image views as well

    for (int i = 0; i < _bootstrapSwapchain.get_images().value().size(); i++)
    {
        _images[i] = _bootstrapSwapchain.get_images().value()[i];
        _imageViews[i] = views[i];
    }
}

void VulkanRenderer::recreateSwapchain()
{
    _device.waitIdle();

    vkb::SwapchainBuilder builder(_bootstrapDevice);
    builder.set_old_swapchain(_bootstrapSwapchain);

    vkb::Result<vkb::Swapchain> result = builder.build();

    if (!result)
    {
        throw std::runtime_error("bk-bootstrap: failed to recreate swapchain");
    }

    vkb::destroy_swapchain(_bootstrapSwapchain);

    for (int i = 0; i < _framebuffers.size(); i++)
    {
        _device.destroyFramebuffer(_framebuffers[i]);
    }

    for (int i = 0; i < _imageViews.size(); i++)
    {
        _device.destroyImageView(_imageViews[i]);
    }

    for (int i = 0; i < _framesInFlight; i++)
    {
        _device.destroySemaphore(_imageReadySemaphores[i]);
        _device.destroySemaphore(_renderCompleteSemaphores[i]);
        _device.destroyFence(_inFlightFences[i]);
    }

    _images.clear();
    _imageViews.clear();

    _bootstrapSwapchain = result.value();
    _swapchain = _bootstrapSwapchain.swapchain;

    _images.resize(_bootstrapSwapchain.get_images().value().size());
    _imageViews.resize(_bootstrapSwapchain.get_images().value().size());

    std::vector<VkImageView> views = _bootstrapSwapchain.get_image_views().value(); // get_image_views apparently creates the image views as well

    for (int i = 0; i < _bootstrapSwapchain.get_images().value().size(); i++)
    {
        _images[i] = _bootstrapSwapchain.get_images().value()[i];
        _imageViews[i] = views[i];
    }

    createFramebuffers();
    createSynchronisationObjects();
}

vk::ShaderModule VulkanRenderer::createShaderModule(const std::vector<uint32_t> code)
{
    vk::ShaderModuleCreateInfo info = {};
    info.sType = vk::StructureType::eShaderModuleCreateInfo;
    info.codeSize = code.size() * sizeof(uint32_t);
    info.pCode = code.data();

    return _device.createShaderModule(info);
}

void VulkanRenderer::createRenderPass()
{
    vk::AttachmentDescription attachmentDescription = {};
    attachmentDescription.format = vk::Format(_bootstrapSwapchain.image_format);
    attachmentDescription.samples = vk::SampleCountFlagBits::e1;
    attachmentDescription.loadOp = vk::AttachmentLoadOp::eClear;
    attachmentDescription.storeOp = vk::AttachmentStoreOp::eStore;
    attachmentDescription.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    attachmentDescription.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    attachmentDescription.initialLayout = vk::ImageLayout::eUndefined;
    attachmentDescription.finalLayout = vk::ImageLayout::ePresentSrcKHR;

    vk::AttachmentReference attachmentReference = {};
    attachmentReference.attachment = 0;
    attachmentReference.layout = vk::ImageLayout::eColorAttachmentOptimal;

    vk::SubpassDescription subpassDescription = {};
    subpassDescription.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    subpassDescription.colorAttachmentCount = 1;
    subpassDescription.pColorAttachments = &attachmentReference;

    vk::RenderPassCreateInfo info = {};
    info.sType = vk::StructureType::eRenderPassCreateInfo;
    info.attachmentCount = 1;
    info.pAttachments = &attachmentDescription;
    info.subpassCount = 1;
    info.pSubpasses = &subpassDescription;

    _renderPass = _device.createRenderPass(info);
}

void VulkanRenderer::createGraphicsPipeline()
{
    std::string vertexShaderCode = readFile("shader.vert");
    std::string fragmentShaderCode = readFile("shader.frag");

    vk::ShaderModule vertexShader = createShaderModule(compileShader("vertexShader", vertexShaderCode, shaderc_shader_kind::shaderc_glsl_vertex_shader, false));
    vk::ShaderModule fragmentShader = createShaderModule(compileShader("fragmentShader", fragmentShaderCode, shaderc_shader_kind::shaderc_glsl_fragment_shader, false));

    vk::PipelineShaderStageCreateInfo shaderStages[2];

    shaderStages[0].sType = vk::StructureType::ePipelineShaderStageCreateInfo;
    shaderStages[0].stage = vk::ShaderStageFlagBits::eVertex;
    shaderStages[0].module = vertexShader;
    shaderStages[0].pName = "main";

    shaderStages[1].sType = vk::StructureType::ePipelineShaderStageCreateInfo;
    shaderStages[1].stage = vk::ShaderStageFlagBits::eFragment;
    shaderStages[1].module = fragmentShader;
    shaderStages[1].pName = "main";

    vk::VertexInputBindingDescription vertexBindingDescription = VulkanVertex::getBindingDescription();
    std::array<vk::VertexInputAttributeDescription, 2> vertexAttributeDescriptions = VulkanVertex::getAttributeDescriptions();

    vk::PipelineVertexInputStateCreateInfo vertexInputStateInfo = {};
    vertexInputStateInfo.sType = vk::StructureType::ePipelineVertexInputStateCreateInfo;
    vertexInputStateInfo.vertexBindingDescriptionCount = 1;
    vertexInputStateInfo.pVertexBindingDescriptions = &vertexBindingDescription;
    vertexInputStateInfo.vertexAttributeDescriptionCount = vertexAttributeDescriptions.size();
    vertexInputStateInfo.pVertexAttributeDescriptions = vertexAttributeDescriptions.data();

    vk::PipelineInputAssemblyStateCreateInfo assemblyStateInfo = {};
    assemblyStateInfo.sType = vk::StructureType::ePipelineInputAssemblyStateCreateInfo;
    assemblyStateInfo.topology = vk::PrimitiveTopology::eTriangleList;
    assemblyStateInfo.primitiveRestartEnable = vk::False;

    vk::Viewport viewport;
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = _bootstrapSwapchain.extent.width;
    viewport.height = _bootstrapSwapchain.extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    vk::Rect2D scissor;
    scissor.offset = vk::Offset2D(0, 0);
    scissor.extent = vk::Extent2D(_bootstrapSwapchain.extent.width, _bootstrapSwapchain.extent.height);

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
    rasterizationStateInfo.frontFace = vk::FrontFace::eClockwise;
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

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = vk::StructureType::ePipelineLayoutCreateInfo;

    _pipelineLayout = _device.createPipelineLayout(pipelineLayoutInfo);

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
    info.renderPass = _renderPass;
    info.subpass = 0;

    auto result = _device.createGraphicsPipeline(nullptr, info);

    if (result.result != vk::Result::eSuccess)
    {
        throw std::runtime_error("vulkan: failed to create graphics pipeline");
    }

    _pipeline = result.value;

    _device.destroyShaderModule(vertexShader);
    _device.destroyShaderModule(fragmentShader);
}

void VulkanRenderer::createFramebuffers()
{
    _framebuffers.resize(_imageViews.size());

    for (int i = 0; i < _framebuffers.size(); i++)
    {
        vk::FramebufferCreateInfo info = {};
        info.sType = vk::StructureType::eFramebufferCreateInfo;
        info.renderPass = _renderPass;
        info.attachmentCount = 1;
        info.pAttachments = &_imageViews[i];
        info.width = _bootstrapSwapchain.extent.width;
        info.height = _bootstrapSwapchain.extent.height;
        info.layers = 1;

        _framebuffers[i] = _device.createFramebuffer(info);
    }
}

void VulkanRenderer::createCommandPool()
{
    vk::CommandPoolCreateInfo info = {};
    info.sType = vk::StructureType::eCommandPoolCreateInfo;
    info.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    info.queueFamilyIndex = _graphicsQueueFamily;
    
    _commandPool = _device.createCommandPool(info);
}

void VulkanRenderer::createVertexBuffer()
{
    const std::vector<VulkanVertex> vertices =
    {
    {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
    {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
    };

    vk::BufferCreateInfo info = {};
    info.sType = vk::StructureType::eBufferCreateInfo;
    info.size = sizeof(vertices[0]) * vertices.size();
    info.usage = vk::BufferUsageFlagBits::eVertexBuffer;
    info.sharingMode = vk::SharingMode::eExclusive;

    _vertexBuffer = _device.createBuffer(info);

    vk::MemoryRequirements requirements = _device.getBufferMemoryRequirements(_vertexBuffer);

    vk::MemoryAllocateInfo allocateInfo = {};
    allocateInfo.sType = vk::StructureType::eMemoryAllocateInfo;
    allocateInfo.allocationSize = requirements.size;
    allocateInfo.memoryTypeIndex = findMemoryType(requirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

    _vertexBufferMemory = _device.allocateMemory(allocateInfo);

    _device.bindBufferMemory(_vertexBuffer, _vertexBufferMemory, 0);

    void* mapped = _device.mapMemory(_vertexBufferMemory, 0, info.size);

    if (mapped == nullptr)
    {
        throw std::runtime_error("vulkan: failed to map memory");
    }

    memcpy(mapped, vertices.data(), info.size);
    _device.unmapMemory(_vertexBufferMemory);
}

void VulkanRenderer::createCommandBuffers()
{
    vk::CommandBufferAllocateInfo info = {};
    info.sType = vk::StructureType::eCommandBufferAllocateInfo;
    info.commandPool = _commandPool;
    info.level = vk::CommandBufferLevel::ePrimary;
    info.commandBufferCount = _framesInFlight;

    _commandBuffers = _device.allocateCommandBuffers(info);
}

void VulkanRenderer::recordCommandBuffer(vk::CommandBuffer buffer, uint32_t image)
{
    vk::CommandBufferBeginInfo bufferBeginInfo = {};
    bufferBeginInfo.sType = vk::StructureType::eCommandBufferBeginInfo;
    
    buffer.begin(bufferBeginInfo);

    vk::RenderPassBeginInfo renderPassBeginInfo = {};
    renderPassBeginInfo.sType = vk::StructureType::eRenderPassBeginInfo;
    renderPassBeginInfo.renderPass = _renderPass;
    renderPassBeginInfo.framebuffer = _framebuffers[image];
    renderPassBeginInfo.renderArea.offset = vk::Offset2D(0, 0);
    renderPassBeginInfo.renderArea.extent = _bootstrapSwapchain.extent;
    
    vk::ClearValue clearValue(vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f));

    renderPassBeginInfo.clearValueCount = 1;
    renderPassBeginInfo.pClearValues = &clearValue;

    buffer.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);

    buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, _pipeline);

    vk::DeviceSize offsets[] = {0};
    buffer.bindVertexBuffers(0, 1, &_vertexBuffer, offsets);

    vk::Viewport viewport;
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = _bootstrapSwapchain.extent.width;
    viewport.height = _bootstrapSwapchain.extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    buffer.setViewport(0, 1, &viewport);

    vk::Rect2D scissor;
    scissor.offset = vk::Offset2D(0, 0);
    scissor.extent = vk::Extent2D(_bootstrapSwapchain.extent.width, _bootstrapSwapchain.extent.height);

    buffer.setScissor(0, 1, &scissor);

    buffer.draw(3, 1, 0, 0);

    buffer.endRenderPass();

    buffer.end();
}

void VulkanRenderer::createSynchronisationObjects()
{
    vk::SemaphoreCreateInfo semaphoreCreateInfo = {};
    semaphoreCreateInfo.sType = vk::StructureType::eSemaphoreCreateInfo;

    vk::FenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.sType = vk::StructureType::eFenceCreateInfo;
    fenceCreateInfo.flags = vk::FenceCreateFlagBits::eSignaled;

    for (int i = 0; i < _framesInFlight; i++)
    {
        _imageReadySemaphores[i] = _device.createSemaphore(semaphoreCreateInfo, nullptr);
        _renderCompleteSemaphores[i] = _device.createSemaphore(semaphoreCreateInfo, nullptr);
        _inFlightFences[i] = _device.createFence(fenceCreateInfo, nullptr);
    }
}
