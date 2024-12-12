#include <SVMV/VulkanBuffer.hxx>

using namespace SVMV;

VulkanBuffer::VulkanBuffer(vk::raii::Device* device, VmaAllocator vmaAllocator, size_t bufferSize, vk::Flags<vk::BufferUsageFlagBits> bufferUsage, VmaMemoryUsage vmaMemoryUsage) : buffer(nullptr)
{
    allocator = vmaAllocator;

    vk::BufferCreateInfo bufferCreateInfo;
    bufferCreateInfo.setSize(bufferSize);
    bufferCreateInfo.setUsage(bufferUsage);

    VmaAllocationCreateInfo vmaAllocationInfo = {};
    vmaAllocationInfo.usage = vmaMemoryUsage;
    vmaAllocationInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VkBuffer temporaryBuffer;
    VkBufferCreateInfo temporaryBufferCreateInfo = bufferCreateInfo;

    VkResult result = vmaCreateBuffer(allocator, &temporaryBufferCreateInfo, &vmaAllocationInfo, &temporaryBuffer, &allocation, nullptr);

    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("vma: failed to create buffer");
    }

    buffer = vk::raii::Buffer(*device, temporaryBuffer);
}

VulkanBuffer::VulkanBuffer(VulkanBuffer&& other) : buffer(nullptr)
{
    this->buffer = std::move(other.buffer);
    this->allocation = other.allocation;
    this->allocator = other.allocator;

    other.allocation = nullptr;
    other.allocator = nullptr;
}

VulkanBuffer& SVMV::VulkanBuffer::operator=(VulkanBuffer&& other)
{
    if (this != &other)
    {
        if (*buffer != nullptr)
        {
            vmaDestroyBuffer(allocator, *buffer, allocation); // NOTE: is this ok with the raii buffer? its destructor is called after...
        }

        this->buffer.clear();

        this->buffer = std::move(other.buffer);
        this->allocation = other.allocation;
        this->allocator = other.allocator;

        other.allocation = nullptr;
        other.allocator = nullptr;
    }

    return *this;
}

VulkanBuffer::~VulkanBuffer()
{
    if (*buffer != nullptr)
    {
        vmaDestroyBuffer(allocator, *buffer, allocation); // NOTE: is this ok with the raii buffer? its destructor is called after, idk
    }
}

VulkanBuffer::operator bool() const
{
    return *buffer != nullptr;
}

const vk::raii::Buffer* VulkanBuffer::getBuffer() const
{
    return &buffer;
}

const VmaAllocator VulkanBuffer::getAllocator() const
{
    return allocator;
}

const VmaAllocation VulkanBuffer::getAllocation() const
{
    return allocation;
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
    : VulkanBuffer(device, destinationBuffer.getAllocator(), destinationBuffer.getAllocation()->GetSize(), vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_TO_GPU)
    , _immediateSubmit(immediateSubmit)
{
    destinationBuffer.getAllocator();
    _capacity = destinationBuffer.getAllocation()->GetSize();
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
    vmaUnmapMemory(allocator, allocation);
}

void VulkanStagingBuffer::pushData()
{
    allocation->GetMappedData();
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
            commandBuffer.copyBuffer(buffer, *destination.getBuffer(), copy);
        });
}
