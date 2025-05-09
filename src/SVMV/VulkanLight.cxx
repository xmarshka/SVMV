#include <SVMV/VulkanLight.hxx>

using namespace SVMV;

VulkanLight::VulkanLight(
    glm::vec4 position, glm::vec4 flux, vk::raii::Device* device, VmaAllocator memoryAllocator, const vk::raii::DescriptorSetLayout& lightDescriptorSetLayout,
    VulkanUtilities::DescriptorAllocator* descriptorAllocator, VulkanDescriptorWriter* descriptorWriter, int framesInFlight
)
    : _descriptorWriter(descriptorWriter)
{;
    lightData.position_0 = position;
    lightData.flux_0 = flux;

    _uniformBuffer = VulkanUniformBuffer(device, memoryAllocator, sizeof(ShaderStructures::LightParametersBuffer));
    _uniformBuffer.setData(&lightData, sizeof(ShaderStructures::LightParametersBuffer));

    for (int i = 0; i < framesInFlight; i++)
    {
        _descriptorSets.push_back(std::move(descriptorAllocator->allocateSet(lightDescriptorSetLayout)));
        _descriptorWriter->writeBuffer(_descriptorSets[i], _uniformBuffer, 0, 0, sizeof(ShaderStructures::LightParametersBuffer), vk::DescriptorType::eUniformBuffer);
    }
}

const vk::raii::DescriptorSet* VulkanLight::getDescriptorSet(int frameIndex) const
{
    if (frameIndex < _descriptorSets.size())
    {
        return &_descriptorSets[frameIndex];
    }
    else
    {
        return nullptr;
    }
}

void VulkanLight::updateLightDescriptor(int frameIndex)
{
    _uniformBuffer.setData(&lightData, sizeof(ShaderStructures::LightParametersBuffer));
    _descriptorWriter->writeBuffer(_descriptorSets[frameIndex], _uniformBuffer, 0, 0, sizeof(ShaderStructures::LightParametersBuffer), vk::DescriptorType::eUniformBuffer);
}
