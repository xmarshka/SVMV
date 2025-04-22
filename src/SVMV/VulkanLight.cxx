#include <SVMV/VulkanLight.hxx>

using namespace SVMV;

VulkanLight::VulkanLight(
    glm::vec3 position, glm::vec3 flux, vk::raii::Device* device, VmaAllocator memoryAllocator, const vk::raii::DescriptorSetLayout& lightDescriptorSetLayout,
    VulkanUtilities::DescriptorAllocator* descriptorAllocator, VulkanDescriptorWriter* descriptorWriter, int framesInFlight
)
    : _descriptorWriter(descriptorWriter)
{
    PointLight lightUniformParameters;
    lightUniformParameters.position = position;
    lightUniformParameters.flux = flux;

    _uniformBuffer = VulkanUniformBuffer(device, memoryAllocator, sizeof(ShaderStructures::LightParametersBuffer));
    _uniformBuffer.setData(&lightUniformParameters, sizeof(ShaderStructures::LightParametersBuffer));

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

void VulkanLight::setLightData(glm::vec3 position, glm::vec3 flux)
{
    PointLight lightUniformParameters;
    lightUniformParameters.position = position;
    lightUniformParameters.flux = flux;

    _uniformBuffer.setData(&lightUniformParameters, sizeof(ShaderStructures::LightParametersBuffer));
}

void VulkanLight::updateLightDescriptor(int frameIndex)
{
    _descriptorWriter->writeBuffer(_descriptorSets[frameIndex], _uniformBuffer, 0, 0, sizeof(ShaderStructures::LightParametersBuffer), vk::DescriptorType::eUniformBuffer);
}
