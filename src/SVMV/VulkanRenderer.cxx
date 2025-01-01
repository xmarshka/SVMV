#include <SVMV/VulkanRenderer.hxx>

using namespace SVMV;

VulkanRenderer::VulkanRenderer(int width, int height, const std::string& name, unsigned framesInFlight)
    : _framesInFlight(framesInFlight)
{
    _instance = _initilization.createInstance(_context, name, 1, 3);
    _messenger = _initilization.createDebugMessenger(_instance);

    _window = VulkanUtilities::GLFWwindowWrapper(name, width, height, this, resized, minimized);
    _surface = _window.createSurface(_instance);

    _physicalDevice = _initilization.createPhysicalDevice(_instance, std::vector<const char*>{ vk::KHRBufferDeviceAddressExtensionName }, _surface);
    _device = _initilization.createDevice(_physicalDevice);
    _swapchain = _initilization.createSwapchain(_device, _surface);
    _swapchainExtent = _initilization.getSwapchainExtent();
    _swapchainFormat = _initilization.getSwapchainFormat();

    _commandPool = _initilization.createCommandPool(_device);

    auto graphicsQueuePair = _initilization.createQueue(_device, vkb::QueueType::graphics);
    _graphicsQueue = std::move(graphicsQueuePair.first);
    _graphicsQueueIndex = graphicsQueuePair.second;

    auto presentQueuePair = _initilization.createQueue(_device, vkb::QueueType::present);
    _presentQueue = std::move(presentQueuePair.first);
    _presentQueueIndex = presentQueuePair.second;

    createRenderPass();

    _imageViews = _initilization.createSwapchainImageViews(_device);
    _framebuffers = _initilization.createFramebuffers(_device, _renderPass, _imageViews);
    _imageReadySemaphores = _initilization.createSemaphores(_device, _framesInFlight);
    _renderCompleteSemaphores = _initilization.createSemaphores(_device, _framesInFlight);
    _inFlightFences = _initilization.createFences(_device, _framesInFlight);

    _descriptorAllocator = VulkanUtilities::DescriptorAllocator(&_device);
    _vmaAllocator = VulkanUtilities::VmaAllocatorWrapper(_instance, _physicalDevice, _device);
    _immediateSubmit = VulkanUtilities::ImmediateSubmit(&_device, &_graphicsQueue, _graphicsQueueIndex);

    vk::CommandBufferAllocateInfo commandBufferAllocateInfo;
    commandBufferAllocateInfo.setLevel(vk::CommandBufferLevel::ePrimary);
    commandBufferAllocateInfo.setCommandPool(_commandPool);
    commandBufferAllocateInfo.setCommandBufferCount(_framesInFlight);

    _drawCommandBuffers = vk::raii::CommandBuffers(_device, commandBufferAllocateInfo);
}

void VulkanRenderer::draw()
{
    vk::Result waitForFencesResult = _device.waitForFences(*_inFlightFences[_activeFrame], vk::True, UINT64_MAX);

    if (waitForFencesResult != vk::Result::eSuccess)
    {
        throw std::runtime_error("vulkan: failure waiting for fences");
    }

    vk::AcquireNextImageInfoKHR acquireNextImageInfo;
    acquireNextImageInfo.setSwapchain(_swapchain);
    acquireNextImageInfo.setTimeout(UINT64_MAX);
    acquireNextImageInfo.setSemaphore(_imageReadySemaphores[_activeFrame]);
    acquireNextImageInfo.setDeviceMask(0b1);

    auto acquireResult = _device.acquireNextImage2KHR(acquireNextImageInfo);

    switch (acquireResult.first)
    {
    case vk::Result::eSuccess:
        break;
    case vk::Result::eErrorOutOfDateKHR:
        recreateSwapchain();
        break;
    default:
        throw std::runtime_error("vulkan: failed to acquire next image");
        break;
    }

    _device.resetFences(*_inFlightFences[_activeFrame]);

    _drawCommandBuffers[_activeFrame].reset();
    recordDrawCommands(_drawCommandBuffers[_activeFrame], _framebuffers[acquireResult.second]);

    vk::SubmitInfo submitInfo;
    submitInfo.setWaitSemaphores(*(_imageReadySemaphores[_activeFrame]));
    vk::PipelineStageFlags waitDstStageFlags = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    submitInfo.setWaitDstStageMask(waitDstStageFlags);
    submitInfo.setCommandBuffers(*(_drawCommandBuffers[_activeFrame]));
    submitInfo.setSignalSemaphores(*(_renderCompleteSemaphores[_activeFrame]));

    _graphicsQueue.submit(submitInfo, *_inFlightFences[_activeFrame]);

    vk::PresentInfoKHR presentInfo;
    presentInfo.setWaitSemaphores(*(_renderCompleteSemaphores[_activeFrame]));
    presentInfo.setSwapchains(*(_swapchain));
    presentInfo.setImageIndices(acquireResult.second);

    auto presentResult = _presentQueue.presentKHR(presentInfo);

    switch (presentResult)
    {
    case vk::Result::eSuccess:
        break;
    case vk::Result::eErrorOutOfDateKHR:
        recreateSwapchain();
        break;
    default:
        throw std::runtime_error("vulkan: failed to acquire next image");
        break;
    }

    _activeFrame = (_activeFrame + 1) % _framesInFlight;
}

void VulkanRenderer::loadScene(std::shared_ptr<Scene> scene)
{
    preprocessScene(scene);
    generateDrawablesFromScene(scene->root, scene->root->transform);
    copyStagingBuffersToGPUBuffers();
}

const vk::Device VulkanRenderer::getDevice() const noexcept
{
    return (*_device);
}

GLFWwindow* VulkanRenderer::getWindow() const noexcept
{
    return _window.getWindow();
}

void VulkanRenderer::recordDrawCommands(const vk::raii::CommandBuffer& commandBuffer, const vk::raii::Framebuffer& framebuffer)
{
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)_swapchainExtent.width / (float)_swapchainExtent.height, 0.1f, 100.0f);

    glm::vec3 cameraPos = glm::vec3(0.9f, 1.1f, 3.0f);
    glm::vec3 cameraFront = glm::vec3(-0.2f, -0.2f, -1.0f);
    glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
    
    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

    ShaderStructures::PushConstants constants;

    commandBuffer.begin(vk::CommandBufferBeginInfo());

    vk::RenderPassBeginInfo renderPassBeginInfo;
    renderPassBeginInfo.setRenderPass(_renderPass);
    renderPassBeginInfo.setFramebuffer(framebuffer);
    renderPassBeginInfo.setRenderArea(vk::Rect2D(vk::Offset2D(0, 0), _swapchainExtent));
    vk::ClearValue clearValue(vk::ClearColorValue(0.2f, 0.2f, 0.2f, 1.0f));
    renderPassBeginInfo.setClearValues(clearValue);

    commandBuffer.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);

    vk::Viewport viewport(0.0f, 0.0f, _swapchainExtent.width, _swapchainExtent.height, 0.0f, 1.0f);
    commandBuffer.setViewport(0, viewport);

    vk::Rect2D scissor(vk::Offset2D(0, 0), _swapchainExtent);
    commandBuffer.setScissor(0, scissor);

    for (const auto& context : _scene.contexts)
    {
        commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *context.second.pipeline);
        commandBuffer.bindIndexBuffer(_scene.indexGpuBuffer.getBuffer(), vk::DeviceSize(0), vk::IndexType::eUint32);

        for (const auto& drawable : context.second.drawables)
        {
            constants.mvpMatrix = projection * view * drawable.modelMatrix;

            constants.addresses.positions = drawable.addresses.positions;
            constants.addresses.normals = drawable.addresses.normals;
            constants.addresses.tangents = drawable.addresses.tangents;
            constants.addresses.texcoords_0 = drawable.addresses.texcoords_0;
            constants.addresses.colors_0 = drawable.addresses.colors_0;

            commandBuffer.pushConstants<ShaderStructures::PushConstants>(*context.second.pipelineLayout, vk::ShaderStageFlagBits::eVertex, 0, constants);
            commandBuffer.drawIndexed(drawable.indexCount, 1, drawable.firstIndex, 0, 0);
        }
    }

    commandBuffer.endRenderPass();
    commandBuffer.end();
}

void VulkanRenderer::preprocessScene(std::shared_ptr<Scene> scene)
{
    std::unordered_map<AttributeType, size_t> attributeSizeMap;
    size_t indexSize = 0;

    for (const auto& mesh : scene->meshes)
    {
        for (const auto& primitive : mesh->primitives)
        {
            indexSize += primitive->indices.size() * sizeof(decltype(primitive->indices)::value_type);

            for (const auto& attribute : primitive->attributes)
            {
                if (attributeSizeMap.find(attribute.attributeType) == attributeSizeMap.end())
                {
                    attributeSizeMap[attribute.attributeType] = attribute.size;
                }
                else
                {
                    attributeSizeMap[attribute.attributeType] += attribute.size;
                }
            }
        }
    }

    if (indexSize <= 0)
    {
        throw std::runtime_error("VulkanRenderer: loaded scene must contain indices");
    }

    _scene.indexGpuBuffer = VulkanGPUBuffer(&_device, _vmaAllocator.getAllocator(), indexSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer);
    _scene.indexStagingBuffer = VulkanStagingBuffer(&_device, _scene.indexGpuBuffer, &_immediateSubmit);

    for (const auto& attributeSize : attributeSizeMap)
    {
        VertexAttribute vertexAttribute;
        vertexAttribute.type = attributeSize.first;
        vertexAttribute.gpuBuffer = VulkanGPUBuffer(&_device, _vmaAllocator.getAllocator(), attributeSize.second, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress);
        vertexAttribute.gpuBufferAddressCounter = vertexAttribute.gpuBuffer.getAddress(_device);
        vertexAttribute.stagingBuffer = VulkanStagingBuffer(&_device, vertexAttribute.gpuBuffer, &_immediateSubmit);

        _scene.attributes.push_back(std::move(vertexAttribute));
    }
}

void VulkanRenderer::generateDrawablesFromScene(std::shared_ptr<Node> node, glm::mat4 baseTransform)
{
    if (node->mesh != nullptr)
    {
        for (const auto& primitive : node->mesh->primitives)
        {
            VulkanDrawable drawable;

            if (_scene.primitiveDrawableMap.find(primitive) != _scene.primitiveDrawableMap.end())
            {
                drawable = _scene.primitiveDrawableMap[primitive];
            }
            else
            {
                drawable.modelMatrix = baseTransform;
                drawable.firstIndex = _scene.indexCounter;
                drawable.indexCount = primitive->indices.size();
                _scene.indexCounter += primitive->indices.size();

                _scene.indexStagingBuffer.pushData(primitive->indices.data(), primitive->indices.size() * sizeof(decltype(primitive->indices)::value_type));

                for (const auto& attribute : primitive->attributes)
                {
                    auto vertexAttributeIterator = std::find_if(_scene.attributes.begin(), _scene.attributes.end(), [&](const VertexAttribute& vertexAttribute) { return vertexAttribute.type == attribute.attributeType; });
                    vertexAttributeIterator->stagingBuffer.pushData(attribute.elements.get(), attribute.size);
                    drawable.setAddress(attribute.attributeType, vertexAttributeIterator->gpuBuffer.getAddress(_device));
                    vertexAttributeIterator->gpuBufferAddressCounter += attribute.size;
                }

                _scene.primitiveDrawableMap[primitive] = drawable;

                // TODO: rewrite necessary
                if (!_scene.contexts.contains(primitive->material->materialTypeName))
                {
                    _scene.contexts[primitive->material->materialTypeName] = VulkanMaterialContext();
                    if (primitive->material->materialTypeName == "glTFPBR")
                    {
                        _scene.glTFPBRMaterial = GLTFPBRMaterial(&_device, _vmaAllocator.getAllocator(), _renderPass, &_descriptorAllocator, _shaderCompiler);
                        _scene.contexts[primitive->material->materialTypeName].pipeline = _scene.glTFPBRMaterial.getPipeline();
                        _scene.contexts[primitive->material->materialTypeName].pipelineLayout = _scene.glTFPBRMaterial.getPipelineLayout();

                        drawable.descriptorSet = _scene.glTFPBRMaterial.createDescriptorSet(primitive->material);
                    }
                    else
                    {
                        throw std::runtime_error("Unsupported material type.");
                    }
                }
            }

            _scene.contexts[primitive->material->materialTypeName].drawables.push_back(drawable);
        }
    }

    for (const auto& child : node->children)
    {
        generateDrawablesFromScene(child, child->transform * baseTransform);
    }
}

void VulkanRenderer::copyStagingBuffersToGPUBuffers()
{
    _scene.indexStagingBuffer.copyToBuffer(_scene.indexGpuBuffer);

    for (auto& attribute : _scene.attributes)
    {
        attribute.stagingBuffer.copyToBuffer(attribute.gpuBuffer);
    }
}

void VulkanRenderer::recreateSwapchain()
{
    //(*_device).waitIdle();

    //vkb::SwapchainBuilder builder(*_physicalDevice, *_device, *_surface, _graphicsQueueIndex, _presentQueueIndex);
    //builder.set_old_swapchain(*_swapchain);

    //vkb::Result<vkb::Swapchain> result = builder.build();
    //if (!result)
    //{
    //    throw std::runtime_error("bk-bootstrap: failed to recreate swapchain");
    //}

    //_imageReadySemaphores.clear();
    //_renderCompleteSemaphores.clear();
    //_inFlightFences.clear();
    //_imageViews.clear();

    //_swapchain.clear();

    //vkb::Swapchain bootstrapSwapchain = result.value();
    //_swapchain = vk::raii::SwapchainKHR(_device, bootstrapSwapchain.swapchain);

    //std::vector<VkImageView> views = bootstrapSwapchain.get_image_views().value(); // get_image_views apparently creates the image views as well
    //for (int i = 0; i < views.size(); i++)
    //{
    //    _imageViews.push_back(vk::raii::ImageView(_device, views[i]));
    //}

    //_swapchainExtent = bootstrapSwapchain.extent;
    //_swapchainFormat = static_cast<vk::Format>(bootstrapSwapchain.image_format);

    //createFramebuffers();
    //createSynchronisationObjects();
}

void VulkanRenderer::createRenderPass()
{
    vk::AttachmentDescription attachmentDescription;
    attachmentDescription.setFormat(_swapchainFormat);
    attachmentDescription.setSamples(vk::SampleCountFlagBits::e1);
    attachmentDescription.setLoadOp(vk::AttachmentLoadOp::eClear);
    attachmentDescription.setStoreOp(vk::AttachmentStoreOp::eStore);
    attachmentDescription.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare);
    attachmentDescription.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);
    attachmentDescription.setInitialLayout(vk::ImageLayout::eUndefined);
    attachmentDescription.setFinalLayout(vk::ImageLayout::ePresentSrcKHR);

    vk::AttachmentReference attachmentReference;
    attachmentReference.setAttachment(0);
    attachmentReference.setLayout(vk::ImageLayout::eColorAttachmentOptimal);

    vk::SubpassDescription subpassDescription;
    subpassDescription.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics);
    subpassDescription.setColorAttachments(attachmentReference);

    vk::RenderPassCreateInfo renderPassCreateInfo;
    renderPassCreateInfo.setAttachments(attachmentDescription);
    renderPassCreateInfo.setSubpasses(subpassDescription);

    _renderPass = vk::raii::RenderPass(_device, renderPassCreateInfo);
}

void VulkanRenderer::resized(GLFWwindow* window, int width, int height)
{
    SVMV::VulkanRenderer* renderer = reinterpret_cast<SVMV::VulkanRenderer*>(glfwGetWindowUserPointer(window));
    renderer->_resized = true;
}

void VulkanRenderer::minimized(GLFWwindow* window, int minimized)
{
    SVMV::VulkanRenderer* renderer = reinterpret_cast<SVMV::VulkanRenderer*>(glfwGetWindowUserPointer(window));
}
