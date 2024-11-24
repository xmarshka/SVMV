#include <SVMV/VulkanUtilities.hxx>

using namespace SVMV;

void VulkanUtilities::ImmediateSubmit::initialize(vk::Device device, vk::Queue queue, unsigned queueFamily)
{
    _device = device;
    _queue = queue;

    vk::CommandPoolCreateInfo poolCreateInfo = {};
    poolCreateInfo.sType = vk::StructureType::eCommandPoolCreateInfo;
    poolCreateInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    poolCreateInfo.queueFamilyIndex = queueFamily;

    _pool = _device.createCommandPool(poolCreateInfo);

    vk::FenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.sType = vk::StructureType::eFenceCreateInfo;
    fenceCreateInfo.flags = vk::FenceCreateFlagBits::eSignaled;

    _fence = _device.createFence(fenceCreateInfo);

    vk::CommandBufferAllocateInfo bufferCreateInfo = {};
    bufferCreateInfo.sType = vk::StructureType::eCommandBufferAllocateInfo;
    bufferCreateInfo.level = vk::CommandBufferLevel::ePrimary;
    bufferCreateInfo.commandPool = _pool;
    bufferCreateInfo.commandBufferCount = 1;

    _buffer = _device.allocateCommandBuffers(bufferCreateInfo)[0];
}

void VulkanUtilities::ImmediateSubmit::free()
{
    if (_pool != nullptr)
    {
        _device.freeCommandBuffers(_pool, _buffer);
        _device.destroyFence(_fence);
        _device.destroyCommandPool(_pool);
    }
}

void VulkanUtilities::ImmediateSubmit::submit(std::function<void(vk::CommandBuffer buffer)>&& lambda)
{
    _device.resetFences(_fence);
    _buffer.reset();

    vk::CommandBufferBeginInfo bufferBeginInfo = {};
    bufferBeginInfo.sType = vk::StructureType::eCommandBufferBeginInfo;

    _buffer.begin(bufferBeginInfo);

    lambda(_buffer);

    _buffer.end();

    vk::SubmitInfo submitInfo = {};
    submitInfo.sType = vk::StructureType::eSubmitInfo;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &_buffer;

    _queue.submit(submitInfo, _fence);
    _device.waitForFences(_fence, true, INT16_MAX);
}
