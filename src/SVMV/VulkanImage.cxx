#include <SVMV/VulkanImage.hxx>

using namespace SVMV;

VulkanImage::VulkanImage(
    vk::raii::Device* device, VulkanUtilities::ImmediateSubmit* immediateSubmit, VmaAllocator vmaAllocator,
    vk::Extent2D extent, vk::Format format, vk::ImageUsageFlags imageUsageFlags, void* data, size_t dataSize
)
    : _device(device), _immediateSubmit(immediateSubmit), _allocator(vmaAllocator), _extent(extent, 1), _format(format)
{
    vk::ImageCreateInfo imageCreateInfo;
    imageCreateInfo.setImageType(vk::ImageType::e2D);
    imageCreateInfo.setFormat(_format);
    imageCreateInfo.setExtent(_extent);
    imageCreateInfo.setMipLevels(1);
    imageCreateInfo.setArrayLayers(1);
    imageCreateInfo.setTiling(vk::ImageTiling::eOptimal);
    imageCreateInfo.setInitialLayout(vk::ImageLayout::eUndefined);
    imageCreateInfo.setSamples(vk::SampleCountFlagBits::e1);
    imageCreateInfo.setUsage(imageUsageFlags);
    imageCreateInfo.setSharingMode(vk::SharingMode::eExclusive);

    VmaAllocationCreateInfo vmaAllocationInfo = {};
    vmaAllocationInfo.usage = VMA_MEMORY_USAGE_AUTO;
    vmaAllocationInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VkImage temporaryImage;
    VkImageCreateInfo temporaryImageCreateInfo = imageCreateInfo;

    VkResult result = vmaCreateImage(_allocator, &temporaryImageCreateInfo, &vmaAllocationInfo, &temporaryImage, &_allocation, nullptr);

    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("vma: failed to create image");
    }

    _image = vk::raii::Image(*_device, temporaryImage);

    vk::ImageViewCreateInfo imageViewCreateInfo;
    imageViewCreateInfo.setViewType(vk::ImageViewType::e2D);
    imageViewCreateInfo.setImage(_image);
    imageViewCreateInfo.setFormat(_format);
    imageViewCreateInfo.setSubresourceRange(vk::ImageSubresourceRange{ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 });

    _imageView = vk::raii::ImageView(*_device, imageViewCreateInfo);

    fillImage(data, dataSize);
}

VulkanImage::VulkanImage(VulkanImage&& other) noexcept
{
    this->_image = std::move(other._image);
    this->_imageView = std::move(other._imageView);
    this->_allocator = other._allocator;
    this->_allocation = other._allocation;
    this->_device = other._device;
    this->_immediateSubmit = other._immediateSubmit;
    this->_format = other._format;
    this->_extent = other._extent;

    other._allocator = nullptr;
    other._allocation = nullptr;
    other._device = nullptr;
    other._immediateSubmit = nullptr;
    other._format = vk::Format::eUndefined;
    other._extent = vk::Extent3D{ 0, 0, 0 };
}

VulkanImage& VulkanImage::operator=(VulkanImage&& other) noexcept
{
    if (this != &other)
    {
        this->_imageView.clear();
        this->_image.clear();

        if (this->_allocator != nullptr && this->_allocation != nullptr)
        {
            vmaFreeMemory(_allocator, _allocation);
        }

        this->_image = std::move(other._image);
        this->_imageView = std::move(other._imageView);
        this->_allocator = other._allocator;
        this->_allocation = other._allocation;
        this->_device = other._device;
        this->_immediateSubmit = other._immediateSubmit;
        this->_format = other._format;
        this->_extent = other._extent;

        other._allocator = nullptr;
        other._allocation = nullptr;
        other._device = nullptr;
        other._immediateSubmit = nullptr;
        other._format = vk::Format::eUndefined;
        other._extent = vk::Extent3D{ 0, 0, 0 };
    }

    return *this;
}

VulkanImage::~VulkanImage()
{
    _imageView.clear();
    _image.clear();

    if (_allocator != nullptr && _allocation != nullptr)
    {
        vmaFreeMemory(_allocator, _allocation);
    }
}

const vk::raii::Image& VulkanImage::getImage() const noexcept
{
    return _image;
}

const vk::raii::ImageView& VulkanImage::getImageView() const noexcept
{
    return _imageView;
}

vk::Format VulkanImage::getFormat() const noexcept
{
    return _format;
}

vk::Extent3D VulkanImage::getExtent() const noexcept
{
    return _extent;
}

VulkanImage::operator bool() const
{
    return *_image != nullptr;
}

void VulkanImage::fillImage(void* data, size_t size)
{
    VulkanStagingBuffer stagingBuffer = VulkanStagingBuffer(_device, _immediateSubmit, _allocator, size);
    stagingBuffer.pushData(data, size);

    vk::ImageMemoryBarrier2 imageMemoryBarrier;
    imageMemoryBarrier.setOldLayout(vk::ImageLayout::eUndefined);
    imageMemoryBarrier.setNewLayout(vk::ImageLayout::eTransferDstOptimal);
    imageMemoryBarrier.setSrcStageMask(vk::PipelineStageFlagBits2::eTopOfPipe);
    imageMemoryBarrier.setSrcAccessMask(vk::AccessFlagBits2::eNone);
    imageMemoryBarrier.setDstStageMask(vk::PipelineStageFlagBits2::eTransfer);
    imageMemoryBarrier.setDstAccessMask(vk::AccessFlagBits2::eMemoryWrite);
    imageMemoryBarrier.setImage(_image);
    imageMemoryBarrier.setSubresourceRange(vk::ImageSubresourceRange{ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 });

    vk::DependencyInfo dependencyInfo;
    dependencyInfo.setImageMemoryBarriers(imageMemoryBarrier);

    vk::BufferImageCopy bufferImageCopy;
    bufferImageCopy.setBufferOffset(0);
    bufferImageCopy.setBufferRowLength(0);
    bufferImageCopy.setBufferImageHeight(0);
    bufferImageCopy.setImageSubresource(vk::ImageSubresourceLayers{ vk::ImageAspectFlagBits::eColor, 0, 0, 1 });
    bufferImageCopy.setImageOffset(vk::Offset3D{ 0, 0, 0 });
    bufferImageCopy.setImageExtent(_extent);

    vk::ImageMemoryBarrier2 imageMemoryBarrierPost = imageMemoryBarrier;
    imageMemoryBarrierPost.setOldLayout(vk::ImageLayout::eTransferDstOptimal);
    imageMemoryBarrierPost.setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
    imageMemoryBarrierPost.setSrcStageMask(vk::PipelineStageFlagBits2::eTransfer);
    imageMemoryBarrierPost.setSrcAccessMask(vk::AccessFlagBits2::eMemoryWrite);
    imageMemoryBarrierPost.setDstStageMask(vk::PipelineStageFlagBits2::eFragmentShader);
    imageMemoryBarrierPost.setDstAccessMask(vk::AccessFlagBits2::eMemoryRead);

    vk::DependencyInfo dependencyInfoPost = dependencyInfo;
    dependencyInfoPost.setImageMemoryBarriers(imageMemoryBarrierPost);

    _immediateSubmit->submit([&](vk::CommandBuffer commandBuffer)
        {
            commandBuffer.pipelineBarrier2(dependencyInfo);
            commandBuffer.copyBufferToImage(stagingBuffer.getBuffer(), _image, vk::ImageLayout::eTransferDstOptimal, bufferImageCopy);
            commandBuffer.pipelineBarrier2(dependencyInfoPost);
        });
}
