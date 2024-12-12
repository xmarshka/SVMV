#include <SVMV/VulkanInitialization.hxx>

using namespace SVMV;

//VulkanInitialization::VulkanInitialization(
//    std::vector<const char*> instanceExtensionNames,
//    std::vector<const char*> deviceExtensionNames,
//    std::vector<const char*> validationLayerNames)
//    : instance(nullptr), physicalDevice(nullptr), device(nullptr), _debugMessenger(nullptr), graphicsQueue(nullptr), presentQueue(nullptr)
//{
//    initialize(instanceExtensionNames, deviceExtensionNames, validationLayerNames);
//}
//
//VulkanInitialization::~VulkanInitialization()
//{
//}
//
//void VulkanInitialization::initialize(std::vector<const char*> instanceExtensionNames, std::vector<const char*> deviceExtensionNames, std::vector<const char*> validationLayerNames)
//{
//    createInstance(instanceExtensionNames, validationLayerNames);
//    createDebugMessenger(); // TODO: only if validation layers are specified?
//    selectPhysicalDevice(deviceExtensionNames);
//    getQueueFamilyIndices();
//    createDevice(deviceExtensionNames);
//    getQueues();
//}
//
//void VulkanInitialization::free()
//{
//}
//
//void VulkanInitialization::createInstance(std::vector<const char*> extensionNames, std::vector<const char*> validationLayerNames)
//{
//    // TODO: make this work for linux and macOS: https://vulkan-tutorial.com/Drawing_a_triangle/Setup/Instance
//    vk::ApplicationInfo applicationInfo;
//    applicationInfo.setPApplicationName("SVMV");
//    applicationInfo.setApplicationVersion(vk::makeVersion(1, 0, 0));
//    applicationInfo.setPEngineName("SVMV");
//    applicationInfo.setEngineVersion(vk::makeVersion(1, 0, 0));
//    applicationInfo.setApiVersion(vk::ApiVersion13);
//
//    vk::InstanceCreateInfo instanceCreateInfo;
//    instanceCreateInfo.setPApplicationInfo(&applicationInfo);
//    instanceCreateInfo.setEnabledExtensionCount(extensionNames.size());
//    instanceCreateInfo.setPEnabledExtensionNames(extensionNames);
//    instanceCreateInfo.setEnabledLayerCount(validationLayerNames.size());
//    instanceCreateInfo.setPEnabledLayerNames(validationLayerNames);
//
//    // TODO: catch EXTENSION_NOT_PRESENT and LAYER_NOT_AVAILABLE(or whatever it is) and let the user know
//    instance = vk::raii::Instance(context, instanceCreateInfo);
//}
//
//void VulkanInitialization::createDebugMessenger()
//{
//    vk::DebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo;
//    debugMessengerCreateInfo.setMessageSeverity(
//        vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
//        vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
//        vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
//    debugMessengerCreateInfo.setMessageType(
//        vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
//        vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
//        vk::DebugUtilsMessageTypeFlagBitsEXT::eDeviceAddressBinding |
//        vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance);
//    debugMessengerCreateInfo.setPfnUserCallback(debugMessageFunction);
//
//    _debugMessenger = vk::raii::DebugUtilsMessengerEXT(instance, debugMessengerCreateInfo);
//}
//
//void VulkanInitialization::selectPhysicalDevice(std::vector<const char*> deviceExtensionNames)
//{
//    vk::raii::PhysicalDevices physicalDevices(instance);
//
//    if (physicalDevices.size() == 0)
//    {
//        throw std::runtime_error("vulkan initialization: failed to find GPU with vulkan support");
//    }
//
//    for (const auto& device : physicalDevices)
//    {
//        // TODO: check for extension support?
//        vk::PhysicalDeviceProperties2 properties = device.getProperties2();
//
//        if (properties.properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
//        {
//            physicalDevice = vk::raii::PhysicalDevice(device);
//            return;
//        }
//    }
//}
//
//void VulkanInitialization::getQueueFamilyIndices()
//{
//    std::vector<vk::QueueFamilyProperties> properties = physicalDevice.getQueueFamilyProperties();
//
//    for (int i = 0; i < properties.size(); i++)
//    {
//        if (properties[i].queueFlags & vk::QueueFlagBits::eGraphics)
//        {
//            graphicsQueueFamily = i;
//        }
//    }
//
//    // TODO: surface support and present queue
//}
//
//void VulkanInitialization::createDevice(std::vector<const char*> deviceExtensionNames)
//{
//    vk::DeviceQueueCreateInfo deviceQueueCreateInfo;
//    deviceQueueCreateInfo.setQueueCount(1);
//    deviceQueueCreateInfo.setQueueFamilyIndex(graphicsQueueFamily);
//    float queuePriority = 1.0f;
//    deviceQueueCreateInfo.setQueuePriorities(queuePriority);
//
//    //vk::PhysicalDeviceVulkan12Features physicalDeviceFeatures;
//    //physicalDeviceFeatures.setBufferDeviceAddress(true);
//
//    vk::PhysicalDeviceFeatures physicalDeviceFeatures;
//
//    vk::DeviceCreateInfo deviceCreateInfo;
//    deviceCreateInfo.setQueueCreateInfoCount(1);
//    deviceCreateInfo.setQueueCreateInfos(deviceQueueCreateInfo);
//    deviceCreateInfo.setPEnabledFeatures(&physicalDeviceFeatures);
//    deviceCreateInfo.setEnabledExtensionCount(deviceExtensionNames.size());
//    deviceCreateInfo.setPEnabledExtensionNames(deviceExtensionNames);
//
//    device = vk::raii::Device(physicalDevice, deviceCreateInfo);
//}
//
//void VulkanInitialization::getQueues()
//{
//    graphicsQueue = device.getQueue(graphicsQueueFamily, 0);
//    presentQueue = device.getQueue(presentQueueFamily, 0);
//}
//
//VKAPI_ATTR VkBool32 VKAPI_CALL VulkanInitialization::debugMessageFunction(
//    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
//    VkDebugUtilsMessageTypeFlagsEXT messageTypes,
//    VkDebugUtilsMessengerCallbackDataEXT const* pCallbackData,
//    void* pUserData)
//{
//    // taken from: https://github.com/KhronosGroup/Vulkan-Hpp/blob/main/samples/EnableValidationWithCallback/EnableValidationWithCallback.cpp
//
//    std::string message;
//    message += vk::to_string(static_cast<vk::DebugUtilsMessageSeverityFlagBitsEXT>(messageSeverity)) + ": " +
//        vk::to_string(static_cast<vk::DebugUtilsMessageTypeFlagsEXT>(messageTypes)) + ":\n";
//    message += std::string("\t") + "messageIDName   = <" + pCallbackData->pMessageIdName + ">\n";
//    message += std::string("\t") + "messageIdNumber = " + std::to_string(pCallbackData->messageIdNumber) + "\n";
//    message += std::string("\t") + "message         = <" + pCallbackData->pMessage + ">\n";
//
//    if (0 < pCallbackData->queueLabelCount)
//    {
//        message += std::string("\t") + "Queue Labels:\n";
//        for (uint32_t i = 0; i < pCallbackData->queueLabelCount; i++)
//        {
//            message += std::string("\t\t") + "labelName = <" + pCallbackData->pQueueLabels[i].pLabelName + ">\n";
//        }
//    }
//    if (0 < pCallbackData->cmdBufLabelCount)
//    {
//        message += std::string("\t") + "CommandBuffer Labels:\n";
//        for (uint32_t i = 0; i < pCallbackData->cmdBufLabelCount; i++)
//        {
//            message += std::string("\t\t") + "labelName = <" + pCallbackData->pCmdBufLabels[i].pLabelName + ">\n";
//        }
//    }
//    if (0 < pCallbackData->objectCount)
//    {
//        for (uint32_t i = 0; i < pCallbackData->objectCount; i++)
//        {
//            message += std::string("\t") + "Object " + std::to_string(i) + "\n";
//            message += std::string("\t\t") + "objectType   = " + vk::to_string(static_cast<vk::ObjectType>(pCallbackData->pObjects[i].objectType)) + "\n";
//            message += std::string("\t\t") + "objectHandle = " + std::to_string(pCallbackData->pObjects[i].objectHandle) + "\n";
//            if (pCallbackData->pObjects[i].pObjectName)
//            {
//                message += std::string("\t\t") + "objectName   = <" + pCallbackData->pObjects[i].pObjectName + ">\n";
//            }
//        }
//    }
//
//    std::cout << message << std::endl;
//
//    return vk::False;
//}

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

    //vk::PhysicalDeviceVulkan12Features features = {};
    //features.sType = vk::StructureType::ePhysicalDeviceVulkan12Features;
    //features.bufferDeviceAddress = true;

    //selector.set_required_features_12(features);

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
    std::vector<vk::raii::ImageView> views;

    std::vector<VkImageView> vkViews = _bootstrapSwapchain.get_image_views().value(); // get_image_views apparently creates the image views as well

    for (int i = 0; i < views.size(); i++)
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
        info.setAttachmentCount(1);
        info.setAttachments(imageViews[i]);
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
