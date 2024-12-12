#include <SVMV/VulkanBuffer.hxx>

using namespace SVMV;

VulkanBuffer::VulkanBuffer(vk::raii::Device* device, VmaAllocator vmaAllocator, size_t bufferSize, vk::Flags<vk::BufferUsageFlagBits> bufferUsage, VmaMemoryUsage vmaMemoryUsage, bool createMapped) : _buffer(nullptr)
{
    _allocator = vmaAllocator;

    vk::BufferCreateInfo bufferCreateInfo;
    bufferCreateInfo.setSize(bufferSize);
    bufferCreateInfo.setUsage(bufferUsage);

    VmaAllocationCreateInfo vmaAllocationInfo = {};
    vmaAllocationInfo.usage = vmaMemoryUsage;
    if (createMapped)
    {
        vmaAllocationInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
    }

    VkBuffer temporaryBuffer = {};
    VkBufferCreateInfo temporaryBufferCreateInfo = bufferCreateInfo;

    VkResult result = vmaCreateBuffer(_allocator, &temporaryBufferCreateInfo, &vmaAllocationInfo, &temporaryBuffer, &_allocation, nullptr);

    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("vma: failed to create buffer");
    }

    _buffer = vk::raii::Buffer(*device, temporaryBuffer);
}

VulkanBuffer::VulkanBuffer(VulkanBuffer&& other) : _buffer(nullptr)
{
    this->_buffer = std::move(other._buffer);
    this->_allocation = other._allocation;
    this->_allocator = other._allocator;

    other._allocation = nullptr;
    other._allocator = nullptr;
}

VulkanBuffer& VulkanBuffer::operator=(VulkanBuffer&& other)
{
    if (this != &other)
    {
        if (*_buffer != nullptr)
        {
            vmaDestroyBuffer(_allocator, *_buffer, _allocation); // NOTE: is this ok with the raii buffer? its destructor is called after...
        }

        this->_buffer = std::move(other._buffer);
        this->_allocation = other._allocation;
        this->_allocator = other._allocator;

        other._allocation = nullptr;
        other._allocator = nullptr;
    }

    return *this;
}

VulkanBuffer::~VulkanBuffer()
{
    if (*_buffer != nullptr)
    {
        vmaDestroyBuffer(_allocator, *_buffer, _allocation); // NOTE: is this ok with the raii buffer? its destructor is called after, idk
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

VulkanGPUBuffer::VulkanGPUBuffer(vk::raii::Device* device, VmaAllocator vmaAllocator, size_t bufferSize, vk::Flags<vk::BufferUsageFlagBits> bufferUsage)
    : VulkanBuffer(device, vmaAllocator, bufferSize, bufferUsage, VMA_MEMORY_USAGE_GPU_ONLY)
{}

VulkanGPUBuffer::VulkanGPUBuffer(VulkanGPUBuffer&& other) noexcept
    : VulkanBuffer(std::move(other))
{}

VulkanGPUBuffer& VulkanGPUBuffer::operator=(VulkanGPUBuffer&& other) noexcept
{
    VulkanBuffer::operator=(std::move(other));
    return *this;
}

VulkanStagingBuffer::VulkanStagingBuffer(vk::raii::Device* device, VulkanUtilities::ImmediateSubmit* immediateSubmit, VmaAllocator vmaAllocator, size_t bufferSize)
    : VulkanBuffer(device, vmaAllocator, bufferSize, vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_ONLY), _immediateSubmit(immediateSubmit)
{
    _capacity = bufferSize;
}

VulkanStagingBuffer::VulkanStagingBuffer(vk::raii::Device* device, const VulkanBuffer& destinationBuffer, VulkanUtilities::ImmediateSubmit* immediateSubmit)
    : VulkanBuffer(device, destinationBuffer.getAllocator(), destinationBuffer.getAllocation()->GetSize(), vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_TO_GPU, true)
    , _immediateSubmit(immediateSubmit)
{
    destinationBuffer.getAllocator();
    _capacity = destinationBuffer.getAllocation()->GetSize();

    VmaAllocationInfo allocationInfo = {};
    vmaGetAllocationInfo(_allocator, _allocation, &allocationInfo);
    _mappedData = reinterpret_cast<uint8_t*>(allocationInfo.pMappedData);
}

VulkanStagingBuffer::VulkanStagingBuffer(VulkanStagingBuffer&& other) noexcept
    : VulkanBuffer(std::move(other))
{
    this->_immediateSubmit = other._immediateSubmit;
    this->_capacity = other._capacity;
    this->_filledSize = other._filledSize;

    other._immediateSubmit = nullptr;
    other._capacity = 0;
    other._filledSize = 0;
}

VulkanStagingBuffer& VulkanStagingBuffer::operator=(VulkanStagingBuffer&& other) noexcept
{
    VulkanBuffer::operator=(std::move(other));

    this->_immediateSubmit = other._immediateSubmit;
    this->_capacity = other._capacity;
    this->_filledSize = other._filledSize;

    other._immediateSubmit = nullptr;
    other._capacity = 0;
    other._filledSize = 0;

    return *this;
}

VulkanStagingBuffer::~VulkanStagingBuffer()
{
    vmaUnmapMemory(_allocator, _allocation);
}

void VulkanStagingBuffer::pushData(void* data, size_t size)
{
    memcpy(_mappedData + _filledSize, data, size);
    _filledSize += size;
}

void VulkanStagingBuffer::copyToBuffer(const VulkanBuffer& destination)
{
    copyToBuffer(destination, destination.getAllocation()->GetSize());
}

void VulkanStagingBuffer::copyToBuffer(const VulkanBuffer& destination, size_t sizeToCopy, size_t offset/* = 0*/)
{
    vk::BufferCopy copy;
    copy.setSize(sizeToCopy);
    copy.setSrcOffset(0);
    copy.setDstOffset(offset);

    _immediateSubmit->submit([&](vk::CommandBuffer commandBuffer)
        {
            commandBuffer.copyBuffer(_buffer, *destination.getBuffer(), copy);
        });
}
