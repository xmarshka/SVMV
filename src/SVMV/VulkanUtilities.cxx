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
    _device->resetFences(*_fence);
    _commandBuffer.reset();

    vk::CommandBufferBeginInfo commandBufferBeginInfo = {};

    _commandBuffer.begin(commandBufferBeginInfo);

    lambda(_commandBuffer);

    _commandBuffer.end();

    vk::SubmitInfo submitInfo;
    submitInfo.setCommandBufferCount(1);
    submitInfo.setCommandBuffers(_commandBuffer);

    _queue->submit(submitInfo, _fence);
    _device->waitForFences(*_fence, true, INT16_MAX);
}

VulkanUtilities::DescriptorAllocator::DescriptorAllocator(vk::raii::Device* device) : _device(device)
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
    info.setSetLayouts(layout);

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
    info.setPoolSizeCount(2);
    info.setPoolSizes(poolSizes);

    std::shared_ptr<vk::raii::DescriptorPool> pool = std::make_shared<vk::raii::DescriptorPool>(*_device, info);

    if (_setsPerPool < 4096)
    {
        _setsPerPool *= 2;
    }

    return pool;
}

VulkanUtilities::GLFWwindowWrapper::GLFWwindowWrapper(const std::string& name, int width, int height, VulkanRenderer* rendererHandle, GLFWframebuffersizefun resizeCallback, GLFWwindowiconifyfun minimizedCallback)
{
    if (glfwInit() != GLFW_TRUE)
    {
        throw std::runtime_error("glfw: failed to initialize glfw");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    _window = glfwCreateWindow(width, height, name.c_str(), nullptr, nullptr);
    if (_window == nullptr)
    {
        throw std::runtime_error("glfw: failed to create window");
    }

    glfwSetWindowUserPointer(_window, this);
    glfwSetFramebufferSizeCallback(_window, resizeCallback);
    glfwSetWindowIconifyCallback(_window, minimizedCallback);
}

VulkanUtilities::GLFWwindowWrapper::GLFWwindowWrapper(GLFWwindowWrapper&& other) noexcept
{
    this->_window = other._window;

    other._window = nullptr;
}

VulkanUtilities::GLFWwindowWrapper& VulkanUtilities::GLFWwindowWrapper::operator=(GLFWwindowWrapper&& other) noexcept
{
    if (this != &other)
    {
        if (_window != nullptr)
        {
            glfwDestroyWindow(_window);
        }

        this->_window = other._window;

        other._window = nullptr;
    }

    return *this;
}

VulkanUtilities::GLFWwindowWrapper::~GLFWwindowWrapper()
{
    glfwDestroyWindow(_window);
}

GLFWwindow* VulkanUtilities::GLFWwindowWrapper::getWindow() const noexcept
{
    return _window;
}

vk::raii::SurfaceKHR VulkanUtilities::GLFWwindowWrapper::createSurface(const vk::raii::Instance& instance) const noexcept
{
    VkSurfaceKHR surface;
    if (glfwCreateWindowSurface(*instance, _window, nullptr, &surface) != VK_SUCCESS)
    {
        throw std::runtime_error("glfw: failed to create window surface");
    }

    return vk::raii::SurfaceKHR(instance, surface);
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
