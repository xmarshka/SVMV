//#include <SVMV/VulkanDescriptorAllocator.hxx>
//
//using namespace SVMV;
//
//VulkanDescriptorAllocator::VulkanDescriptorAllocator(vk::raii::Device* device)
//{
//    _device = device;
//}
//
//VulkanDescriptorAllocator::VulkanDescriptorAllocator(VulkanDescriptorAllocator&& other) noexcept
//{
//    this->_device = other._device;
//    this->_availablePools = other._availablePools;
//    this->_filledPools = other._filledPools;
//
//    other._device = nullptr;
//    other._availablePools.clear();
//    other._filledPools.clear();
//}
//
//VulkanDescriptorAllocator& VulkanDescriptorAllocator::operator=(VulkanDescriptorAllocator&& other) noexcept
//{
//    if (this != &other)
//    {
//        this->destroyPools();
//
//        this->_device = other._device;
//        this->_availablePools = other._availablePools;
//        this->_filledPools = other._filledPools;
//
//        other._device = nullptr;
//        other._availablePools.clear();
//        other._filledPools.clear();
//    }
//}
//
//void VulkanDescriptorAllocator::destroyPools()
//{
//    _availablePools.clear();
//    _filledPools.clear();
//}
//
//void SVMV::VulkanDescriptorAllocator::clearPools()
//{
//    for (const auto& pool : _availablePools)
//    {
//        pool->reset();
//    }
//
//    for (const auto& pool : _filledPools)
//    {
//        pool->reset();
//        _availablePools.push_back(pool);
//    }
//
//    _filledPools.clear();
//}
//
//vk::raii::DescriptorSet VulkanDescriptorAllocator::allocateSet(const vk::raii::DescriptorSetLayout& layout)
//{
//    std::shared_ptr<vk::raii::DescriptorPool> pool = getPool();
//
//    vk::DescriptorSetAllocateInfo info = {};
//    info.setDescriptorPool(*pool);
//    info.setDescriptorSetCount(1);
//    info.setSetLayouts(layout);
//
//    vk::raii::DescriptorSets sets(nullptr);
//
//    try
//    {
//        sets = vk::raii::DescriptorSets(*_device, info);
//    }
//    catch (const vk::SystemError& e)
//    {
//        if (e.code() == vk::make_error_code(vk::Result::eErrorOutOfPoolMemory) || e.code() == vk::make_error_code(vk::Result::eErrorFragmentedPool))
//        {
//            _filledPools.push_back(pool);
//
//            pool = getPool();
//            info.descriptorPool = *pool;
//
//            sets = vk::raii::DescriptorSets(*_device, info);
//        }
//        else
//        {
//            throw std::runtime_error("VulkanDescriptorAllocator: failed to allocate descriptor set");
//        }
//    }
//
//    _availablePools.push_back(pool);
//
//    return std::move(sets[0]);
//}
//
//std::shared_ptr<vk::raii::DescriptorPool> VulkanDescriptorAllocator::getPool()
//{
//    std::shared_ptr<vk::raii::DescriptorPool> pool;
//
//    if (_availablePools.size() != 0)
//    {
//        pool = _availablePools.back();
//        _availablePools.pop_back();
//    }
//    else
//    {
//        pool = createPool();
//        _availablePools.push_back(pool);
//    }
//
//    return pool;
//}
//
//std::shared_ptr<vk::raii::DescriptorPool> VulkanDescriptorAllocator::createPool()
//{
//    vk::DescriptorPoolSize poolSizes[2];
//    poolSizes[0].setType(vk::DescriptorType::eUniformBuffer);
//    poolSizes[0].setDescriptorCount(_setsPerPool);
//
//    poolSizes[1].setType(vk::DescriptorType::eCombinedImageSampler);
//    poolSizes[1].setDescriptorCount(_setsPerPool);
//
//    vk::DescriptorPoolCreateInfo info = {};
//    info.setMaxSets(_setsPerPool);
//    info.setPoolSizeCount(2);
//    info.setPoolSizes(poolSizes);
//
//    std::shared_ptr<vk::raii::DescriptorPool> pool = std::make_shared<vk::raii::DescriptorPool>(*_device, info);
//
//    if (_setsPerPool < 4096)
//    {
//        _setsPerPool *= 2;
//    }
//
//    return pool;
//}

// TODO: UNUSED
