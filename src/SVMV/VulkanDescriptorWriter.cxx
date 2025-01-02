#include <SVMV/VulkanDescriptorWriter.hxx>

using namespace SVMV;

VulkanDescriptorWriter::VulkanDescriptorWriter(vk::raii::Device* device)
    : _device(device)
{}

VulkanDescriptorWriter::VulkanDescriptorWriter(VulkanDescriptorWriter&& other) noexcept
{
    this->_device = other._device;

    other._device = nullptr;
}

VulkanDescriptorWriter& VulkanDescriptorWriter::operator=(VulkanDescriptorWriter&& other) noexcept
{
    if (this != &other)
    {
        this->_device = other._device;

        other._device = nullptr;
    }

    return *this;
}

void VulkanDescriptorWriter::writeBuffer(const vk::raii::DescriptorSet& descriptorSet, const VulkanBuffer& buffer, int binding, int offset, int size, vk::DescriptorType bufferType)
{
    vk::DescriptorBufferInfo descriptorBufferInfo;
    descriptorBufferInfo.setBuffer(buffer.getBuffer());
    descriptorBufferInfo.setOffset(offset);
    descriptorBufferInfo.setRange(size);

    vk::WriteDescriptorSet writeDescriptorSet;
    writeDescriptorSet.setDstBinding(binding);
    writeDescriptorSet.setDstSet(descriptorSet);
    writeDescriptorSet.setDescriptorCount(1);
    writeDescriptorSet.setDescriptorType(bufferType);
    writeDescriptorSet.setBufferInfo(descriptorBufferInfo);

    _device->updateDescriptorSets(writeDescriptorSet, nullptr);
}
