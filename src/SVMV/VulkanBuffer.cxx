#include <SVMV/VulkanBuffer.hxx>
#include <vk_mem_alloc.h>

using namespace SVMV;

VulkanBuffer::VulkanBuffer() : allocator(nullptr), buffer(nullptr), allocation(nullptr)
{
}

void VulkanBuffer::create(VmaAllocator vmaAllocator, size_t bufferSize, vk::Flags<vk::BufferUsageFlagBits> bufferUsage, VmaMemoryUsage vmaMemoryUsage)
{
    allocator = vmaAllocator;

    vk::BufferCreateInfo vertexBufferCreateInfo = {};
    vertexBufferCreateInfo.sType = vk::StructureType::eBufferCreateInfo;
    vertexBufferCreateInfo.size = bufferSize;
    vertexBufferCreateInfo.usage = bufferUsage;

    VmaAllocationCreateInfo vmaAllocationInfo = {};
    vmaAllocationInfo.usage = vmaMemoryUsage;
    vmaAllocationInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VkBuffer temp;
    VkBufferCreateInfo temp2 = vertexBufferCreateInfo;

    VkResult result = vmaCreateBuffer(allocator, &temp2, &vmaAllocationInfo, &temp, &allocation, &info);

    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("vma: failed to create buffer");
    }

    buffer = temp;
}

VulkanBuffer::~VulkanBuffer()
{
    if (buffer != nullptr)
    {
        vmaDestroyBuffer(allocator, buffer, allocation);
    }
}

VulkanBuffer::operator bool() const
{
    return buffer != nullptr;
}
