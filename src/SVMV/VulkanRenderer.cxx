#include <SVMV/VulkanRenderer.hxx>

using namespace SVMV;

VulkanRenderer::VulkanRenderer(int width, int height, const std::string& name, unsigned framesInFlight)
    : _framesInFlight(framesInFlight)
{
    _instance = _initilization.createInstance(_context, name, 1, 3);
    _messenger = _initilization.createDebugMessenger(_instance);

    _window = VulkanUtilities::GLFWwindowWrapper(name, width, height, this, (GLFWframebuffersizefun)resized, (GLFWwindowiconifyfun)minimized);
    _surface = _window.createSurface(_instance);

    _physicalDevice = _initilization.createPhysicalDevice(_instance, std::vector<const char*>{ vk::KHRBufferDeviceAddressExtensionName }, _surface);
    _device = _initilization.createDevice(_physicalDevice);
    _swapchain = _initilization.createSwapchain(_device, _surface);
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
}

void VulkanRenderer::draw()
{
    (*_device).waitForFences(1, &(*_inFlightFences[_activeFrame]), vk::True, UINT64_MAX);

    auto result = (*_device).acquireNextImageKHR((*_swapchain), UINT64_MAX, _imageReadySemaphores[_activeFrame]);

    if (result.result == vk::Result::eErrorOutOfDateKHR || _resized)
    {
        _resized = false;
        recreateSwapchain();
        return;
    }
    else if (result.result != vk::Result::eSuccess)
    {
        throw std::runtime_error("vulkan: failed to acquire next image");
    }

    (*_device).resetFences(1, &(*_inFlightFences[_activeFrame]));

    unsigned imageIndex = result.value;

    auto vector = _scene.recordFrameCommandBuffers(_activeFrame, _framebuffers[imageIndex], _swapchainExtent.width, _swapchainExtent.height);

    vk::SubmitInfo submitInfo = {};
    submitInfo.sType = vk::StructureType::eSubmitInfo;

    vk::Semaphore waitSemaphores[] = { (*_imageReadySemaphores[_activeFrame]) };
    vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput }; // TODO

    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = vector.size();
    submitInfo.pCommandBuffers = vector.data();

    vk::Semaphore signalSemaphores[] = { (*_renderCompleteSemaphores[_activeFrame]) };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if ((*_graphicsQueue).submit(1, &submitInfo, (*_inFlightFences[_activeFrame])) != vk::Result::eSuccess)
    {
        throw std::runtime_error("vulkan: failed to submit to graphics queue");
    }

    vk::PresentInfoKHR presentInfo = {};
    presentInfo.sType = vk::StructureType::ePresentInfoKHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &(*_swapchain);
    presentInfo.pImageIndices = &imageIndex;

    vk::Result presentResult = (*_presentQueue).presentKHR(presentInfo);

    if (presentResult == vk::Result::eErrorOutOfDateKHR || /*presentResult != vk::Result::eSuboptimalKHR || */_resized)
    {
        _resized = false;
        recreateSwapchain();
    }
    else if (presentResult != vk::Result::eSuccess)
    {
        throw std::runtime_error("vulkan: failed to present image");
    }

    _activeFrame = (_activeFrame + 1) % _framesInFlight;
}

void VulkanRenderer::loadScene(std::shared_ptr<Scene> scene)
{
    preprocessScene(scene);
    generateDrawablesFromScene(scene->root, glm::mat4(1.0f));
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

void VulkanRenderer::preprocessScene(std::shared_ptr<Scene> scene)
{
    size_t indicesBufferSize = 0;

    size_t positionsBufferSize = 0;
    size_t normalsBufferSize = 0;
    size_t tangentsBufferSize = 0;
    size_t texcoords_0BufferSize = 0;
    size_t colors_0BufferSize = 0;

    for (const auto& mesh : scene->meshes)
    {
        for (const auto& primitive : mesh->primitives)
        {
            indicesBufferSize += primitive->indices.size() * sizeof(decltype(primitive->indices)::value_type);

            positionsBufferSize += primitive->positions.size() * sizeof(decltype(primitive->positions)::value_type);
            normalsBufferSize += primitive->normals.size() * sizeof(decltype(primitive->normals)::value_type);
            tangentsBufferSize += primitive->tangents.size() * sizeof(decltype(primitive->tangents)::value_type);
            texcoords_0BufferSize += primitive->texcoords_0.size() * sizeof(decltype(primitive->texcoords_0)::value_type);
            colors_0BufferSize += primitive->colors_0.size() * sizeof(decltype(primitive->colors_0)::value_type);
        }
    }

    _scene.indices.gpuBuffer = VulkanGPUBuffer(&_device, _vmaAllocator.getAllocator(), indicesBufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer);
    _scene.indices.stagingBuffer = VulkanStagingBuffer(&_device, _scene.indices.gpuBuffer, &_immediateSubmit);

    if (positionsBufferSize > 0)
    {
        _scene.positions.gpuBuffer = VulkanGPUBuffer(&_device, _vmaAllocator.getAllocator(), positionsBufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress);
        _scene.positions.gpuBufferAddressCounter = _scene.positions.gpuBuffer.getAddress(_device);
        _scene.positions.stagingBuffer = VulkanStagingBuffer(&_device, _scene.positions.gpuBuffer, &_immediateSubmit);
    }
    if (normalsBufferSize > 0)
    {
        _scene.normals.gpuBuffer = VulkanGPUBuffer(&_device, _vmaAllocator.getAllocator(), normalsBufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress);
        _scene.normals.gpuBufferAddressCounter = _scene.normals.gpuBuffer.getAddress(_device);
        _scene.normals.stagingBuffer = VulkanStagingBuffer(&_device, _scene.positions.gpuBuffer, &_immediateSubmit);
    }
    if (tangentsBufferSize > 0)
    {
        _scene.tangents.gpuBuffer = VulkanGPUBuffer(&_device, _vmaAllocator.getAllocator(), tangentsBufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress);
        _scene.tangents.gpuBufferAddressCounter = _scene.tangents.gpuBuffer.getAddress(_device);
        _scene.tangents.stagingBuffer = VulkanStagingBuffer(&_device, _scene.positions.gpuBuffer, &_immediateSubmit);
    }
    if (texcoords_0BufferSize > 0)
    {
        _scene.texcoords_0.gpuBuffer = VulkanGPUBuffer(&_device, _vmaAllocator.getAllocator(), texcoords_0BufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress);
        _scene.texcoords_0.gpuBufferAddressCounter = _scene.texcoords_0.gpuBuffer.getAddress(_device);
        _scene.texcoords_0.stagingBuffer = VulkanStagingBuffer(&_device, _scene.positions.gpuBuffer, &_immediateSubmit);
    }
    if (colors_0BufferSize > 0)
    {
        _scene.colors_0.gpuBuffer = VulkanGPUBuffer(&_device, _vmaAllocator.getAllocator(), colors_0BufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress);
        _scene.colors_0.gpuBufferAddressCounter = _scene.colors_0.gpuBuffer.getAddress(_device);
        _scene.colors_0.stagingBuffer = VulkanStagingBuffer(&_device, _scene.positions.gpuBuffer, &_immediateSubmit);
    }
}

void VulkanRenderer::generateDrawablesFromScene(std::shared_ptr<Node> node, glm::mat4 baseTransform)
{
    for (const auto& primitive : node->mesh->primitives)
    {
        VulkanDrawable drawable;

        drawable.modelMatrix = node->transform * baseTransform;
        drawable.firstIndex = _scene.indexCounter;
        drawable.indexCount = primitive->indices.size();

        _scene.indices.stagingBuffer.pushData(primitive->indices.data(), primitive->indices.size() * sizeof(decltype(primitive->indices)::value_type));

        if (primitive->positions.empty() == false)
        {
            _scene.positions.stagingBuffer.pushData(primitive->positions.data(), primitive->positions.size() * sizeof(decltype(primitive->positions)::value_type));
            drawable.addresses.positions = _scene.positions.gpuBufferAddressCounter;
            _scene.positions.gpuBufferAddressCounter += primitive->positions.size() * sizeof(decltype(primitive->positions)::value_type);
        }
        if (primitive->normals.empty() == false)
        {
            _scene.normals.stagingBuffer.pushData(primitive->normals.data(), primitive->normals.size() * sizeof(decltype(primitive->normals)::value_type));
            drawable.addresses.normals = _scene.normals.gpuBufferAddressCounter;
            _scene.normals.gpuBufferAddressCounter += primitive->normals.size() * sizeof(decltype(primitive->normals)::value_type);
        }
        if (primitive->tangents.empty() == false)
        {
            _scene.tangents.stagingBuffer.pushData(primitive->tangents.data(), primitive->tangents.size() * sizeof(decltype(primitive->tangents)::value_type));
            drawable.addresses.tangents = _scene.tangents.gpuBufferAddressCounter;
            _scene.tangents.gpuBufferAddressCounter += primitive->tangents.size() * sizeof(decltype(primitive->tangents)::value_type);
        }
        if (primitive->texcoords_0.empty() == false)
        {
            _scene.texcoords_0.stagingBuffer.pushData(primitive->texcoords_0.data(), primitive->texcoords_0.size() * sizeof(decltype(primitive->texcoords_0)::value_type));
            drawable.addresses.texcoords_0 = _scene.texcoords_0.gpuBufferAddressCounter;
            _scene.texcoords_0.gpuBufferAddressCounter += primitive->texcoords_0.size() * sizeof(decltype(primitive->texcoords_0)::value_type);
        }
        if (primitive->colors_0.empty() == false)
        {
            _scene.colors_0.stagingBuffer.pushData(primitive->colors_0.data(), primitive->colors_0.size() * sizeof(decltype(primitive->colors_0)::value_type));
            drawable.addresses.colors_0 = _scene.colors_0.gpuBufferAddressCounter;
            _scene.colors_0.gpuBufferAddressCounter += primitive->colors_0.size() * sizeof(decltype(primitive->colors_0)::value_type);
        }
    }
}

void VulkanRenderer::copyStagingBuffersToGPUBuffers()
{
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
    vk::AttachmentDescription attachmentDescription = {};
    attachmentDescription.format = _swapchainFormat;
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

    _renderPass = vk::raii::RenderPass(_device, info);
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
