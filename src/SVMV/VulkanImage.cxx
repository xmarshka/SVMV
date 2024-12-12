#include <SVMV/VulkanImage.hxx>

using namespace SVMV;

VulkanImage::VulkanImage() : image(nullptr), imageView(nullptr), _allocator(nullptr), allocation(nullptr), info({}), format(vk::Format::eUndefined)
{
}

VulkanImage::VulkanImage(vk::raii::Device* device, VmaAllocator vmaAllocator, unsigned width, unsigned height, vk::Format format, vk::Flags<vk::ImageUsageFlagBits> imageUsage, VulkanUtilities::ImmediateSubmit* immediateSubmit)
    : image(nullptr), imageView(nullptr)
{
    create(device, vmaAllocator, width, height, format, imageUsage, immediateSubmit);
}

VulkanImage::~VulkanImage()
{
    free();
}

void VulkanImage::create(vk::raii::Device* device, VmaAllocator vmaAllocator, unsigned width, unsigned height, vk::Format format, vk::Flags<vk::ImageUsageFlagBits> imageUsage, VulkanUtilities::ImmediateSubmit* immediateSubmit)
{
    _allocator = vmaAllocator;
    _device = device;
    _immediateSubmit = immediateSubmit;
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

    VkImage temporaryImage;
    VkImageCreateInfo temporaryImageCreateInfo = imageCreateInfo;

    VkResult result = vmaCreateImage(_allocator, &temporaryImageCreateInfo, &vmaAllocationInfo, &temporaryImage, &allocation, &info);

    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("vma: failed to create image");
    }

    image = vk::raii::Image(*_device, temporaryImage);

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

    imageView = vk::raii::ImageView(*_device, imageViewCreateInfo);
}

void VulkanImage::copyDataToImage(void* data, size_t dataSize)
{
    /*if (*image != nullptr)
    {
        VulkanStagingBuffer stagingBuffer = VulkanStagingBuffer(_device, _immediateSubmit, _allocator, dataSize);

        memcpy(info.pMappedData, data, dataSize);

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

        vk::BufferImageCopy copyRegion = {};
        copyRegion.bufferOffset = 0;
        copyRegion.bufferRowLength = 0;
        copyRegion.bufferImageHeight = 0;

        copyRegion.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        copyRegion.imageSubresource.mipLevel = 0;
        copyRegion.imageSubresource.baseArrayLayer = 0;
        copyRegion.imageSubresource.layerCount = 1;
        copyRegion.imageExtent = extent;

        _immediateSubmit->submit([&](vk::CommandBuffer buffer)
            {
                buffer.pipelineBarrier2(dependencyInfo);
                buffer.copyBufferToImage(stagingBuffer.buffer, image, vk::ImageLayout::eTransferDstOptimal, copyRegion);
            });

        vk::ImageMemoryBarrier2 imageBarrier2 = imageBarrier;
        imageBarrier2.oldLayout = vk::ImageLayout::eTransferDstOptimal;
        imageBarrier2.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

        dependencyInfo.pImageMemoryBarriers = &imageBarrier2;

        _immediateSubmit->submit([&](vk::CommandBuffer buffer)
            {
                buffer.pipelineBarrier2(dependencyInfo);
            });
    }*/
}

void VulkanImage::free()
{
    if (*image != nullptr)
    {
        vmaDestroyImage(_allocator, *image, allocation);
    }
}

VulkanImage::operator bool() const
{
    return *image != nullptr;
}
