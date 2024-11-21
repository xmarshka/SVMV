#include <SVMV/VulkanImage.hxx>

using namespace SVMV;

VulkanImage::VulkanImage() : device(nullptr), allocator(nullptr), image(nullptr), imageView(nullptr), allocation(nullptr), info()
{
}

void VulkanImage::create(vk::Device device, VmaAllocator vmaAllocator, unsigned width, unsigned height, vk::Format format, vk::Flags<vk::ImageUsageFlagBits> imageUsage)
{
    allocator = vmaAllocator;
    device = device;
    extent.width = width;
    extent.height = height;
    extent.depth = 1;
    format = format;

    vk::ImageCreateInfo imageCreateInfo = {};
    imageCreateInfo.sType = vk::StructureType::eImageCreateInfo;
    imageCreateInfo.imageType = vk::ImageType::e2D;
    imageCreateInfo.format = format;
    imageCreateInfo.extent = extent;
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.tiling = vk::ImageTiling::eOptimal;
    imageCreateInfo.initialLayout = vk::ImageLayout::eUndefined;
    imageCreateInfo.samples = vk::SampleCountFlagBits::e1;
    imageCreateInfo.usage = imageUsage;
    imageCreateInfo.sharingMode = vk::SharingMode::eExclusive;

    VmaAllocationCreateInfo vmaAllocationInfo = {};
    vmaAllocationInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    vmaAllocationInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VkImage temp;
    VkImageCreateInfo temp2 = imageCreateInfo;

    VkResult result = vmaCreateImage(allocator, &temp2, &vmaAllocationInfo, &temp, &allocation, &info);

    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("vma: failed to create image");
    }

    image = temp;

    vk::ImageViewCreateInfo imageViewCreateInfo = {};
    imageViewCreateInfo.sType = vk::StructureType::eImageViewCreateInfo;
    imageViewCreateInfo.viewType = vk::ImageViewType::e2D;
    imageViewCreateInfo.image = image;
    imageViewCreateInfo.format = format;

    imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
    imageViewCreateInfo.subresourceRange.levelCount = 1;
    imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    imageViewCreateInfo.subresourceRange.layerCount = 1;
    imageViewCreateInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;

    imageView = device.createImageView(imageViewCreateInfo);
}

void VulkanImage::copyDataToImage(void* data, size_t dataSize, vk::CommandBuffer commandBuffer, vk::Queue queue)
{
    if (image == nullptr)
    {
        return;
    }

    VulkanBuffer stagingBuffer;
    stagingBuffer.create(allocator, dataSize, vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_ONLY);

    memcpy(info.pMappedData, data, dataSize);

    commandBuffer.reset();
    
    vk::ImageMemoryBarrier2 imageBarrier = {};
    imageBarrier.sType = vk::StructureType::eImageMemoryBarrier2;
    imageBarrier.oldLayout = vk::ImageLayout::eUndefined;
    imageBarrier.newLayout = vk::ImageLayout::eTransferDstOptimal;

    imageBarrier.srcStageMask = vk::PipelineStageFlagBits2::eAllCommands;
    imageBarrier.srcAccessMask = vk::AccessFlagBits2::eMemoryWrite;
    imageBarrier.dstStageMask = vk::PipelineStageFlagBits2::eAllCommands;
    imageBarrier.dstAccessMask = vk::AccessFlagBits2::eMemoryWrite | vk::AccessFlagBits2::eMemoryRead;

    imageBarrier.image = image;
    imageBarrier.subresourceRange.baseMipLevel = 0;
    imageBarrier.subresourceRange.levelCount = 1;
    imageBarrier.subresourceRange.baseArrayLayer = 0;
    imageBarrier.subresourceRange.layerCount = 1;
    imageBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;

    vk::DependencyInfo dependencyInfo = {};
    dependencyInfo.sType = vk::StructureType::eDependencyInfo;
    dependencyInfo.imageMemoryBarrierCount = 1;
    dependencyInfo.pImageMemoryBarriers = &imageBarrier;

    commandBuffer.pipelineBarrier2(dependencyInfo);

    vk::BufferImageCopy copyRegion = {};
    copyRegion.bufferOffset = 0;
    copyRegion.bufferRowLength = 0;
    copyRegion.bufferImageHeight = 0;

    copyRegion.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    copyRegion.imageSubresource.mipLevel = 0;
    copyRegion.imageSubresource.baseArrayLayer = 0;
    copyRegion.imageSubresource.layerCount = 1;
    copyRegion.imageExtent = extent;

    commandBuffer.copyBufferToImage(stagingBuffer.buffer, image, vk::ImageLayout::eTransferDstOptimal, copyRegion);

    vk::ImageMemoryBarrier2 imageBarrier2 = imageBarrier;
    imageBarrier2.oldLayout = vk::ImageLayout::eTransferDstOptimal;
    imageBarrier2.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

    dependencyInfo.pImageMemoryBarriers = &imageBarrier2;

    commandBuffer.pipelineBarrier2(dependencyInfo);

    vk::SubmitInfo submitInfo = {};
    submitInfo.sType = vk::StructureType::eSubmitInfo;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    queue.submit(submitInfo);
    queue.waitIdle();
}

void VulkanImage::free()
{
    if (image != nullptr)
    {
        device.destroyImageView(imageView);
        vmaDestroyImage(allocator, image, allocation);
    }
}

VulkanImage::operator bool() const
{
    return image != nullptr;
}
