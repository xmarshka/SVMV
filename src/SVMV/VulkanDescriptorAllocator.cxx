#include <SVMV/VulkanDescriptorAllocator.hxx>

using namespace SVMV;

void VulkanDescriptorAllocator::initialize(vk::Device device)
{
    _device = device;

    _setsPerPool = 8; // should be a power of 2
}

void VulkanDescriptorAllocator::destroyPools()
{
    for (const auto& pool : _availablePools)
    {
        _device.destroyDescriptorPool(pool);
    }
    _availablePools.clear();

    for (const auto& pool : _filledPools)
    {
        _device.destroyDescriptorPool(pool);
    }
    _filledPools.clear();
}

void SVMV::VulkanDescriptorAllocator::clearPools()
{
    for (const auto& pool : _availablePools)
    {
        _device.resetDescriptorPool(pool);
    }

    for (const auto& pool : _filledPools)
    {
        _device.resetDescriptorPool(pool);
        _availablePools.push_back(pool);
    }

    _filledPools.clear();
}

vk::DescriptorSet VulkanDescriptorAllocator::allocateSet(vk::DescriptorSetLayout layout)
{
    vk::DescriptorPool pool = getPool();

    vk::DescriptorSetAllocateInfo info = {};
    info.sType = vk::StructureType::eDescriptorSetAllocateInfo;
    info.descriptorPool = pool;
    info.descriptorSetCount = 1;
    info.pSetLayouts = &layout;

    vk::DescriptorSet set;

    vk::Result result = _device.allocateDescriptorSets(&info, &set);

    if (result == vk::Result::eErrorOutOfPoolMemory || result == vk::Result::eErrorFragmentedPool)
    {
        _filledPools.push_back(pool);

        pool = getPool();
        info.descriptorPool = pool;

        set = _device.allocateDescriptorSets(info)[0];
    }

    _availablePools.push_back(pool);

    return set;
}

vk::DescriptorPool VulkanDescriptorAllocator::getPool()
{
    vk::DescriptorPool pool;

    if (_availablePools.size() != 0)
    {
        pool = _availablePools.back();
        _availablePools.pop_back();
    }
    else
    {
        pool = createPool();
        _availablePools.push_back(pool);
    }

    return pool;
}

vk::DescriptorPool VulkanDescriptorAllocator::createPool()
{
    vk::DescriptorPoolSize poolSizes[2];
    poolSizes[0].type = vk::DescriptorType::eUniformBuffer;
    poolSizes[0].descriptorCount = _setsPerPool;

    poolSizes[1].type = vk::DescriptorType::eCombinedImageSampler;
    poolSizes[1].descriptorCount = _setsPerPool;

    vk::DescriptorPoolCreateInfo info = {};
    info.sType = vk::StructureType::eDescriptorPoolCreateInfo;
    info.maxSets = _setsPerPool;
    info.poolSizeCount = 2;
    info.pPoolSizes = poolSizes;

    vk::DescriptorPool pool = _device.createDescriptorPool(info);

    if (_setsPerPool < 4096)
    {
        _setsPerPool *= 2;
    }

    return pool;
}
