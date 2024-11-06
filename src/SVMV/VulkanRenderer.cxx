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
    createFramebuffers();
    createCommandPool();
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

    auto vector = _scene->recordFrameCommandBuffers(_activeFrame, _framebuffers[imageIndex], _bootstrapSwapchain.extent.width, _bootstrapSwapchain.extent.height);

    vk::SubmitInfo submitInfo = {};
    submitInfo.sType = vk::StructureType::eSubmitInfo;

    vk::Semaphore waitSemaphores[] = { _imageReadySemaphores[_activeFrame] };
    vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput }; // TODO

    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

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
