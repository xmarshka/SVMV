#include <SVMV/VulkanBuffer.hxx>

using namespace SVMV;

VulkanBuffer::VulkanBuffer(vk::raii::Device* device, VmaAllocator vmaAllocator, size_t bufferSize, vk::Flags<vk::BufferUsageFlagBits> bufferUsage, bool enableWriting/* = false*/)
{
    _allocator = vmaAllocator;

    vk::BufferCreateInfo bufferCreateInfo;
    bufferCreateInfo.setSize(bufferSize);
    bufferCreateInfo.setUsage(bufferUsage);

    VmaAllocationCreateInfo vmaAllocationInfo = {};
    vmaAllocationInfo.usage = VMA_MEMORY_USAGE_AUTO;
    if (enableWriting)
    {
        vmaAllocationInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
    }

    VkBuffer temporaryBuffer = {};
    VkBufferCreateInfo temporaryBufferCreateInfo = bufferCreateInfo;

    VkResult result = vmaCreateBuffer(_allocator, &temporaryBufferCreateInfo, &vmaAllocationInfo, &temporaryBuffer, &_allocation, nullptr);

    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("vma: failed to create buffer");
    }

    _buffer = vk::raii::Buffer(*device, temporaryBuffer);
    _size = bufferSize;
}

VulkanBuffer::VulkanBuffer(VulkanBuffer&& other) noexcept
{
    this->_buffer = std::move(other._buffer);
    this->_allocator = other._allocator;
    this->_allocation = other._allocation;
    this->_size = other._size;

    other._allocator = nullptr;
    other._allocation = nullptr;
    other._size = 0;
}

VulkanBuffer& VulkanBuffer::operator=(VulkanBuffer&& other) noexcept
{
    if (this != &other)
    {
        this->_buffer.clear();

        if (this->_allocator != nullptr && this->_allocation != nullptr)
        {
            vmaFreeMemory(_allocator, _allocation);
        }

        this->_buffer = std::move(other._buffer);
        this->_allocator = other._allocator;
        this->_allocation = other._allocation;
        this->_size = other._size;

        other._allocator = nullptr;
        other._allocation = nullptr;
        other._size = 0;
    }

    return *this;
}

VulkanBuffer::~VulkanBuffer()
{
    this->_buffer.clear();

    if (_allocator != nullptr && _allocation != nullptr)
    {
        vmaFreeMemory(_allocator, _allocation);
    }
}

VulkanBuffer::operator bool() const
{
    return *_buffer != nullptr;
}

const vk::raii::Buffer& VulkanBuffer::getBuffer() const
{
    return _buffer;
}

const VmaAllocator VulkanBuffer::getAllocator() const
{
    return _allocator;
}

const VmaAllocation VulkanBuffer::getAllocation() const
{
    return _allocation;
}

const size_t VulkanBuffer::getSize() const
{
    return _size;
}

const vk::DeviceAddress VulkanBuffer::getAddress(const vk::Device& device) const
{
    vk::BufferDeviceAddressInfo deviceAddressInfo;
    deviceAddressInfo.buffer = _buffer;

    return device.getBufferAddress(deviceAddressInfo);
}

VulkanGPUBuffer::VulkanGPUBuffer(vk::raii::Device* device, VmaAllocator vmaAllocator, size_t bufferSize, vk::Flags<vk::BufferUsageFlagBits> bufferUsage)
    : VulkanBuffer(device, vmaAllocator, bufferSize, bufferUsage)
{}

VulkanGPUBuffer::VulkanGPUBuffer(VulkanGPUBuffer&& other) noexcept
    : VulkanBuffer(std::move(other))
{}

VulkanGPUBuffer& VulkanGPUBuffer::operator=(VulkanGPUBuffer&& other) noexcept
{
    if (this != &other)
    {
        VulkanBuffer::operator=(std::move(other));
    }

    return *this;
}

VulkanStagingBuffer::VulkanStagingBuffer(vk::raii::Device* device, VulkanUtilities::ImmediateSubmit* immediateSubmit, VmaAllocator vmaAllocator, size_t bufferSize)
    : VulkanBuffer(device, vmaAllocator, bufferSize, vk::BufferUsageFlagBits::eTransferSrc, true), _immediateSubmit(immediateSubmit)
{
    _capacity = bufferSize;

    vmaMapMemory(_allocator, _allocation, reinterpret_cast<void**>(&_mappedData));
}

VulkanStagingBuffer::VulkanStagingBuffer(vk::raii::Device* device, const VulkanBuffer& destinationBuffer, VulkanUtilities::ImmediateSubmit* immediateSubmit)
    : VulkanBuffer(device, destinationBuffer.getAllocator(), destinationBuffer.getSize(), vk::BufferUsageFlagBits::eTransferSrc, true), _immediateSubmit(immediateSubmit)
{
    _capacity = destinationBuffer.getSize();

    vmaMapMemory(_allocator, _allocation, reinterpret_cast<void**>(&_mappedData));
}

VulkanStagingBuffer::VulkanStagingBuffer(VulkanStagingBuffer&& other) noexcept
    : VulkanBuffer(std::move(other))
{
    this->_mappedData = other._mappedData;
    this->_immediateSubmit = other._immediateSubmit;
    this->_capacity = other._capacity;
    this->_filledSize = other._filledSize;

    other._mappedData = 0;
    other._immediateSubmit = nullptr;
    other._capacity = 0;
    other._filledSize = 0;
}

VulkanStagingBuffer& VulkanStagingBuffer::operator=(VulkanStagingBuffer&& other) noexcept
{
    if (this != &other)
    {
        if (this->_allocator != nullptr && this->_allocation != nullptr)
        {
            vmaUnmapMemory(this->_allocator, this->_allocation);
        }

        VulkanBuffer::operator=(std::move(other));

        this->_mappedData = other._mappedData;
        this->_immediateSubmit = other._immediateSubmit;
        this->_capacity = other._capacity;
        this->_filledSize = other._filledSize;

        other._mappedData = 0;
        other._immediateSubmit = nullptr;
        other._capacity = 0;
        other._filledSize = 0;
    }

    return *this;
}

VulkanStagingBuffer::~VulkanStagingBuffer()
{
    if (_allocator != nullptr && _allocation != nullptr)
    {
        vmaUnmapMemory(_allocator, _allocation);
    }
}

void VulkanStagingBuffer::pushData(void* data, size_t size)
{
    memcpy(_mappedData + _filledSize, data, size);
    _filledSize += size;
}

void VulkanStagingBuffer::resetDataPointer()
{
    _filledSize = 0;
}

void VulkanStagingBuffer::copyToBuffer(const VulkanBuffer& destination)
{
    copyToBuffer(destination, _size);
}

void VulkanStagingBuffer::copyToBuffer(const VulkanBuffer& destination, size_t sizeToCopy, size_t offset/* = 0*/)
{
    vk::BufferCopy copy;
    copy.setSize(sizeToCopy);
    copy.setSrcOffset(0);
    copy.setDstOffset(offset);

    // TODO: have this wait for fence for the copy

    /*vk::raii::Fence* fence = */_immediateSubmit->submit([&](vk::CommandBuffer commandBuffer)
        {
            commandBuffer.copyBuffer(_buffer, *destination.getBuffer(), copy);
        });

    /*vk::Result waitForFencesResult = _device->waitForFences(**fence, true, INT32_MAX);

    if (waitForFencesResult != vk::Result::eSuccess)
    {
        throw std::runtime_error("vulkan: failure waiting for fences");
    }*/
}

VulkanUniformBuffer::VulkanUniformBuffer(vk::raii::Device* device, VmaAllocator vmaAllocator, size_t bufferSize)
    : VulkanBuffer(device, vmaAllocator, bufferSize, vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eUniformBuffer, true)
{
    vmaMapMemory(_allocator, _allocation, reinterpret_cast<void**>(&_mappedData));
}

VulkanUniformBuffer::VulkanUniformBuffer(VulkanUniformBuffer&& other) noexcept
    : VulkanBuffer(std::move(other))
{
    this->_mappedData = other._mappedData;

    other._mappedData = nullptr;
}

VulkanUniformBuffer& VulkanUniformBuffer::operator=(VulkanUniformBuffer&& other) noexcept
{
    if (this != &other)
    {
        if (this->_allocator != nullptr && this->_allocation != nullptr)
        {
            vmaUnmapMemory(this->_allocator, this->_allocation);
        }

        VulkanBuffer::operator=(std::move(other));

        this->_mappedData = other._mappedData;

        other._mappedData = nullptr;
    }

    return *this;
}

VulkanUniformBuffer::~VulkanUniformBuffer()
{
    if (_allocator != nullptr && _allocation != nullptr)
    {
        vmaUnmapMemory(_allocator, _allocation);
    }
}

void VulkanUniformBuffer::setData(void* data, size_t size)
{
    if (size == _size)
    {
        memcpy(_mappedData, data, size);
    }
}
