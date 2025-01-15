#include <SVMV/VulkanInitialization.hxx>

using namespace SVMV;

vk::raii::Instance VulkanInitilization::createInstance(const vk::raii::Context& context, const std::string& name, unsigned apiVersionMajor, unsigned apiVersionMinor)
{
    vkb::InstanceBuilder builder;
    builder.set_app_name(name.c_str());
    builder.set_engine_name(name.c_str());
    builder.request_validation_layers();
    builder.use_default_debug_messenger();
    builder.desire_api_version(apiVersionMajor, apiVersionMinor);

    vkb::Result<vkb::Instance> result = builder.build();
    if (!result)
    {
        throw std::runtime_error("bk-bootstrap: failed to build instance");
    }

    _bootstrapInstance = result.value();

    return vk::raii::Instance(context, _bootstrapInstance.instance);
}

vk::raii::DebugUtilsMessengerEXT VulkanInitilization::createDebugMessenger(const vk::raii::Instance& instance)
{
    return vk::raii::DebugUtilsMessengerEXT(instance, _bootstrapInstance.debug_messenger);
}

vk::raii::PhysicalDevice VulkanInitilization::createPhysicalDevice(const vk::raii::Instance& instance, std::vector<const char*> extensions, const vk::raii::SurfaceKHR& surface)
{
    vkb::PhysicalDeviceSelector selector(_bootstrapInstance, *surface);

    for (const auto& extension : extensions)
    {
        selector.add_required_extension(extension);
    }

    // TODO: is this the right way to do this?
    vk::PhysicalDeviceVulkan12Features features12;
    features12.setBufferDeviceAddress(true);

    selector.set_required_features_12(features12);

    vkb::Result<vkb::PhysicalDevice> physicalDeviceResult = selector.select();
    if (!physicalDeviceResult)
    {
        throw std::runtime_error("bk-bootstrap: failed to select physical device");
    }

    _bootstrapPhysicalDevice = physicalDeviceResult.value();
    return vk::raii::PhysicalDevice(instance, _bootstrapPhysicalDevice.physical_device);
}

vk::raii::Device VulkanInitilization::createDevice(const vk::raii::PhysicalDevice& physicalDevice)
{
    vkb::DeviceBuilder builder(_bootstrapPhysicalDevice);

    vkb::Result<vkb::Device> deviceResult = builder.build();
    if (!deviceResult)
    {
        throw std::runtime_error("bk-bootstrap: failed to create device");
    }

    _bootstrapDevice = deviceResult.value();
    return vk::raii::Device(physicalDevice, _bootstrapDevice.device);
}

vk::raii::SwapchainKHR VulkanInitilization::createSwapchain(const vk::raii::Device& device, const vk::raii::SurfaceKHR& surface)
{
    vkb::SwapchainBuilder builder(_bootstrapDevice, *surface);

    // TODO: set buffering amount

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
    return vk::raii::SwapchainKHR(device, _bootstrapSwapchain.swapchain);
}

vk::raii::SwapchainKHR VulkanInitilization::recreateSwapchain(const vk::raii::Device& device, const vk::raii::SurfaceKHR& surface)
{
    return vk::raii::SwapchainKHR(nullptr);
}

vk::raii::CommandPool VulkanInitilization::createCommandPool(const vk::raii::Device& device)
{
    vk::CommandPoolCreateInfo info = {};
    info.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
    info.setQueueFamilyIndex(_bootstrapDevice.get_queue_index(vkb::QueueType::graphics).value());

    return vk::raii::CommandPool(device, info);
}

std::pair<vk::raii::Queue, unsigned> VulkanInitilization::createQueue(const vk::raii::Device& device, vkb::QueueType queueType)
{
    return std::pair<vk::raii::Queue, unsigned>(vk::raii::Queue(device, _bootstrapDevice.get_queue(queueType).value()), _bootstrapDevice.get_queue_index(vkb::QueueType::graphics).value());
}

std::vector<vk::raii::ImageView> VulkanInitilization::createSwapchainImageViews(const vk::raii::Device& device)
{
    std::vector<VkImageView> vkViews = _bootstrapSwapchain.get_image_views().value(); // get_image_views apparently creates the image views as well
    std::vector<vk::raii::ImageView> views;

    for (int i = 0; i < vkViews.size(); i++)
    {
        views.push_back(vk::raii::ImageView(device, vkViews[i]));
    }

    return views;
}

std::vector<vk::raii::Framebuffer> VulkanInitilization::createFramebuffers(const vk::raii::Device& device, const vk::raii::RenderPass& renderPass, const std::vector<vk::raii::ImageView>& imageViews)
{
    std::vector<vk::raii::Framebuffer> framebuffers;

    for (int i = 0; i < imageViews.size(); i++)
    {
        vk::FramebufferCreateInfo info;
        info.setRenderPass(renderPass);
        info.setAttachments(*(imageViews[i]));
        info.setWidth(_bootstrapSwapchain.extent.width);
        info.setHeight(_bootstrapSwapchain.extent.height);
        info.setLayers(1);

        framebuffers.push_back(vk::raii::Framebuffer(device, info));
    }

    return framebuffers;
}

std::vector<vk::raii::Framebuffer> VulkanInitilization::createFramebuffers(const vk::raii::Device& device, const vk::raii::RenderPass& renderPass, const std::vector<vk::raii::ImageView>& imageViews, const vk::raii::ImageView& depthImageView)
{
    std::vector<vk::raii::Framebuffer> framebuffers;

    std::array<vk::ImageView, 2> attachments = { nullptr, *depthImageView };

    for (int i = 0; i < imageViews.size(); i++)
    {
        attachments[0] = *(imageViews[i]);

        vk::FramebufferCreateInfo info;
        info.setRenderPass(renderPass);
        info.setAttachments(attachments);
        info.setWidth(_bootstrapSwapchain.extent.width);
        info.setHeight(_bootstrapSwapchain.extent.height);
        info.setLayers(1);

        framebuffers.push_back(vk::raii::Framebuffer(device, info));
    }

    return framebuffers;
}

std::vector<vk::raii::Semaphore> VulkanInitilization::createSemaphores(const vk::raii::Device& device, int count)
{
    std::vector<vk::raii::Semaphore> semaphores;

    vk::SemaphoreCreateInfo semaphoreCreateInfo;
    
    for (int i = 0; i < count; i++)
    {
        semaphores.push_back(vk::raii::Semaphore(device, semaphoreCreateInfo));
    }

    return semaphores;
}

std::vector<vk::raii::Fence> VulkanInitilization::createFences(const vk::raii::Device& device, int count)
{
    std::vector<vk::raii::Fence> fences;

    vk::FenceCreateInfo fenceCreateInfo;
    fenceCreateInfo.setFlags(vk::FenceCreateFlagBits::eSignaled);

    for (int i = 0; i < count; i++)
    {
        fences.push_back(vk::raii::Fence(device, fenceCreateInfo));
    }

    return fences;
}

vk::Extent2D VulkanInitilization::getSwapchainExtent()
{
    return vk::Extent2D(_bootstrapSwapchain.extent);
}

vk::Format VulkanInitilization::getSwapchainFormat()
{
    return vk::Format(_bootstrapSwapchain.image_format);
}
