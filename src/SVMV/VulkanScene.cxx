//#include <SVMV/VulkanScene.hxx>
//#include <vk_mem_alloc.h>
//
//using namespace SVMV;
//
//namespace {
//    std::unordered_map<std::shared_ptr<Primitive>, VulkanDrawable> primitiveDrawableMap;
//}
//
//VulkanScene::VulkanScene() : _allocator(nullptr), _queueIndex(0)
//{
//}
//
//VulkanScene::VulkanScene(vk::raii::PhysicalDevice* physicalDevice, vk::raii::Instance* instance, vk::raii::Device* device, vk::raii::CommandPool* commandPool, vk::raii::Queue* queue, unsigned queueIndex)
//{
//    initialize(physicalDevice, instance, device, commandPool, queue, queueIndex);
//}
//
//VulkanScene::~VulkanScene()
//{
//    free();
//}
//
//void VulkanScene::initialize(vk::raii::PhysicalDevice* physicalDevice, vk::raii::Instance* instance, vk::raii::Device* device, vk::raii::CommandPool* commandPool, vk::raii::Queue* queue, unsigned queueIndex)
//{
//    _device = device;
//    _commandPool = commandPool;
//    _queue = queue;
//    _queueIndex = queueIndex;
//
//    VmaAllocatorCreateInfo allocatorInfo = {};
//    allocatorInfo.physicalDevice = **physicalDevice;
//    allocatorInfo.device = **device;
//    allocatorInfo.instance = **instance;
//    allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
//
//    vmaCreateAllocator(&allocatorInfo, &_allocator);
//
//    // TODO: inject immediateSubmit, descriptorAllocator
//}
//
//void VulkanScene::free()
//{
//    primitiveDrawableMap.clear();
//
//    _collectionMap.clear();
//
//    if (_allocator)
//    {
//        vmaDestroyAllocator(_allocator);
//    }
//}
//
//void VulkanScene::setScene(std::shared_ptr<Scene> scene, vk::raii::RenderPass* renderPass, const vk::Extent2D& extent, unsigned int framesInFlight)
//{
//    primitiveDrawableMap.clear();
//    _renderPass = renderPass;
//
//    //_GLTFPBRMaterial.free();
//    _GLTFPBRMaterial.initialize(_device, _allocator, *_renderPass, extent, _descriptorAllocator);
//
//    createCollectionsFromScene(scene);
//    createDrawablesFromScene(scene, scene->root);
//
//    for (auto& collection : _collectionMap)
//    {
//        copyStagingBufferDataToBuffers(collection.second);
//    }
//
//    createCollectionCommandBuffers(framesInFlight);
//
//    // TODO: two passes:
//    //          1: prepare collections (get total vertex size, create the collections) (loop only through scene->meshes)
//    //          2: load scene to memory (including buffers, keep offsets and create complete drawables)
//    //
//    //          NOTES: sources not necessarry?
//}
//
//void VulkanScene::createCollectionsFromScene(std::shared_ptr<Scene> scene)
//{
//    _collectionMap[MaterialName::GLTFPBR] = std::make_unique<VulkanDrawableCollection>();
//    _collectionMap[MaterialName::GLTFPBR]->materialPipeline = _GLTFPBRMaterial.getMaterialPipeline();
//
//    std::unique_ptr<VulkanDrawableCollection>& collection = _collectionMap[MaterialName::GLTFPBR];
//
//    for (const auto& mesh : scene->meshes)
//    {
//        for (const auto& primitive : mesh->primitives)
//        {
//            collection->indices.count += primitive->indices.size();
//
//            collection->positions.count += primitive->positions.size();
//            collection->normals.count += primitive->normals.size();
//            collection->tangents.count += primitive->tangents.size();
//            collection->texcoords_0.count += primitive->texcoords_0.size();
//            collection->colors_0.count += primitive->colors_0.size();
//        }
//    }
//
//    auto bufferUsage = vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eShaderDeviceAddress;
//
//    for (auto& collection : _collectionMap)
//    {
//        collection.second->indices.buffer.create(_device, _allocator, collection.second->indices.count * sizeof(uint32_t), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer, VMA_MEMORY_USAGE_GPU_ONLY);
//        collection.second->indices.stagingBuffer.create(_device, _allocator, collection.second->indices.count * sizeof(uint32_t), vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_ONLY);
//
//        vk::BufferDeviceAddressInfo deviceAddressInfo = {};
//        deviceAddressInfo.sType = vk::StructureType::eBufferDeviceAddressInfo;
//        deviceAddressInfo.buffer = collection.second->indices.buffer.buffer;
//
//        if (collection.second->positions.count > 0)
//        {
//            collection.second->positions.buffer.create(_device, _allocator, collection.second->positions.count * 4 * sizeof(float), bufferUsage, VMA_MEMORY_USAGE_GPU_ONLY);
//            collection.second->positions.stagingBuffer.create(_device, _allocator, collection.second->positions.count * 4 * sizeof(float), vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_ONLY);
//            deviceAddressInfo.buffer = collection.second->positions.buffer.buffer;
//
//            collection.second->positions.bufferAddress = (**_device).getBufferAddress(deviceAddressInfo);
//        }
//        if (collection.second->normals.count > 0)
//        {
//            collection.second->normals.buffer.create(_device, _allocator, collection.second->normals.count * 4 * sizeof(float), bufferUsage, VMA_MEMORY_USAGE_GPU_ONLY);
//            collection.second->normals.stagingBuffer.create(_device, _allocator, collection.second->normals.count * 4 * sizeof(float), vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_ONLY);
//            deviceAddressInfo.buffer = collection.second->normals.buffer.buffer;
//
//            collection.second->normals.bufferAddress = (**_device).getBufferAddress(deviceAddressInfo);
//        }
//        if (collection.second->tangents.count > 0)
//        {
//            collection.second->tangents.buffer.create(_device, _allocator, collection.second->tangents.count * 4 * sizeof(float), bufferUsage, VMA_MEMORY_USAGE_GPU_ONLY);
//            collection.second->tangents.stagingBuffer.create(_device, _allocator, collection.second->tangents.count * 4 * sizeof(float), vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_ONLY);
//            deviceAddressInfo.buffer = collection.second->tangents.buffer.buffer;
//
//            collection.second->tangents.bufferAddress = (**_device).getBufferAddress(deviceAddressInfo);
//        }
//        if (collection.second->texcoords_0.count > 0)
//        {
//            collection.second->texcoords_0.buffer.create(_device, _allocator, collection.second->texcoords_0.count * 2 * sizeof(float), bufferUsage, VMA_MEMORY_USAGE_GPU_ONLY);
//            collection.second->texcoords_0.stagingBuffer.create(_device, _allocator, collection.second->texcoords_0.count * 2 * sizeof(float), vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_ONLY);
//            deviceAddressInfo.buffer = collection.second->texcoords_0.buffer.buffer;
//
//            collection.second->texcoords_0.bufferAddress = (**_device).getBufferAddress(deviceAddressInfo);
//        }
//        if (collection.second->colors_0.count > 0)
//        {
//            collection.second->colors_0.buffer.create(_device, _allocator, collection.second->colors_0.count * 4 * sizeof(float), bufferUsage, VMA_MEMORY_USAGE_GPU_ONLY);
//            collection.second->colors_0.stagingBuffer.create(_device, _allocator, collection.second->colors_0.count * 4 * sizeof(float), vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_ONLY);
//            deviceAddressInfo.buffer = collection.second->colors_0.buffer.buffer;
//
//            collection.second->colors_0.bufferAddress = (**_device).getBufferAddress(deviceAddressInfo);
//        }
//    }
//}
//
void VulkanScene::createDrawablesFromScene(std::shared_ptr<Scene> scene, std::shared_ptr<Node> rootNode)
{
    if (rootNode->mesh)
    {
        for (const auto& primitive : rootNode->mesh->primitives)
        {
            VulkanDrawable drawable;
            drawable.modelMatrix = rootNode->transform;

            // TODO: change this to not force GLTFPBR
            std::unique_ptr<VulkanDrawableCollection>& collection = _collectionMap[MaterialName::GLTFPBR];

            if (primitiveDrawableMap.find(primitive) == primitiveDrawableMap.end()) // this primitive has not been processed yet
            {
                drawable.firstIndex = collection->indices.dataOffset; // TODO: this needs to be set properly, requires a seperate field in collection probably
                drawable.indexCount = primitive->indices.size();

                //drawable.materialInstance = _GLTFPBRMaterial.generateMaterialInstance(primitive->material, _descriptorAllocator);

                drawable.addresses.positions = collection->positions.bufferAddress + collection->positions.dataOffset * sizeof(float);
                drawable.addresses.normals = collection->normals.bufferAddress + collection->normals.dataOffset * sizeof(float);
                drawable.addresses.tangents = collection->tangents.bufferAddress + collection->tangents.dataOffset * sizeof(float);
                drawable.addresses.texcoords_0 = collection->texcoords_0.bufferAddress + collection->texcoords_0.dataOffset * sizeof(float);
                drawable.addresses.colors_0 = collection->colors_0.bufferAddress + collection->colors_0.dataOffset * sizeof(float);

                loadPrimitiveAttributeDataToBuffer(collection, primitive);

                primitiveDrawableMap[primitive] = drawable;
            }
            else
            {
                drawable.firstIndex = primitiveDrawableMap[primitive].firstIndex;
                drawable.indexCount = primitiveDrawableMap[primitive].indexCount;

                drawable.materialInstance = primitiveDrawableMap[primitive].materialInstance;
            }

            collection->drawables.emplace_back(drawable);
        }
    }

    for (const auto& childNode : rootNode->children)
    {
        createDrawablesFromScene(scene, childNode);
    }
}
//
//void VulkanScene::loadPrimitiveAttributeDataToBuffer(const std::unique_ptr<VulkanDrawableCollection>& collection, std::shared_ptr<Primitive> primitive) // TODO: make this accept MaterialName instead of the collection ptr reference
//{
//    void* mappedData = nullptr;
//    vmaMapMemory(_allocator, collection->indices.stagingBuffer.allocation, &mappedData);
//    copyIndicesData(reinterpret_cast<uint32_t*>(mappedData), primitive->indices, collection->indices.dataOffset);
//    vmaUnmapMemory(_allocator, collection->indices.stagingBuffer.allocation);
//
//    if (primitive->positions.size() > 0)
//    {
//        vmaMapMemory(_allocator, collection->positions.stagingBuffer.allocation, &mappedData);
//        float* data = reinterpret_cast<float*>(mappedData);
//        copyAttributeData<float, float, 3>(data, primitive->positions, collection->positions.dataOffset);
//        vmaUnmapMemory(_allocator, collection->positions.stagingBuffer.allocation);
//    }
//    if (primitive->normals.size() > 0)
//    {
//        vmaMapMemory(_allocator, collection->normals.stagingBuffer.allocation, &mappedData);
//        float* data = reinterpret_cast<float*>(mappedData);
//        copyAttributeData<float, float, 3>(data, primitive->normals, collection->normals.dataOffset);
//        vmaUnmapMemory(_allocator, collection->normals.stagingBuffer.allocation);
//    }
//    if (primitive->tangents.size() > 0)
//    {
//        vmaMapMemory(_allocator, collection->tangents.stagingBuffer.allocation, &mappedData);
//        float* data = reinterpret_cast<float*>(mappedData);
//        copyAttributeData<float, float, 4>(data, primitive->tangents, collection->tangents.dataOffset);
//        vmaUnmapMemory(_allocator, collection->tangents.stagingBuffer.allocation);
//    }
//    if (primitive->texcoords_0.size() > 0)
//    {
//        vmaMapMemory(_allocator, collection->texcoords_0.stagingBuffer.allocation, &mappedData);
//        float* data = reinterpret_cast<float*>(mappedData);
//        copyAttributeData<float, float, 2>(data, primitive->texcoords_0, collection->texcoords_0.dataOffset);
//        vmaUnmapMemory(_allocator, collection->texcoords_0.stagingBuffer.allocation);
//    }
//    if (primitive->colors_0.size() > 0)
//    {
//        vmaMapMemory(_allocator, collection->colors_0.stagingBuffer.allocation, &mappedData);
//        float* data = reinterpret_cast<float*>(mappedData);
//        copyAttributeData<float, float, 4>(data, primitive->colors_0, collection->colors_0.dataOffset);
//        vmaUnmapMemory(_allocator, collection->colors_0.stagingBuffer.allocation);
//    }
//}
//
//void VulkanScene::copyStagingBufferDataToBuffers(const std::unique_ptr<VulkanDrawableCollection>& collection)
//{
//    // TODO: don't immediate submit, create a command buffer here and submit manually
//
//    vk::CommandBufferAllocateInfo commandBufferInfo = {};
//    commandBufferInfo.sType = vk::StructureType::eCommandBufferAllocateInfo;
//    commandBufferInfo.level = vk::CommandBufferLevel::ePrimary;
//    commandBufferInfo.commandPool = **_commandPool;
//    commandBufferInfo.commandBufferCount = 1;
//
//    vk::CommandBuffer commandBuffer = (**_device).allocateCommandBuffers(commandBufferInfo)[0];
//
//    vk::CommandBufferBeginInfo commandBufferBeginInfo = {};
//    commandBufferBeginInfo.sType = vk::StructureType::eCommandBufferBeginInfo;
//    commandBufferBeginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
//
//    commandBuffer.begin(commandBufferBeginInfo);
//
//    vk::BufferCopy indicesCopyRegion = {};
//    indicesCopyRegion.size = collection->indices.count * sizeof(uint32_t);
//    commandBuffer.copyBuffer(collection->indices.stagingBuffer.buffer, collection->indices.buffer.buffer, 1, &indicesCopyRegion);
//
//    if (collection->positions.buffer)
//    {
//        vk::BufferCopy positionsCopyRegion = {};
//        positionsCopyRegion.size = collection->positions.count * 4 * sizeof(float);
//        commandBuffer.copyBuffer(collection->positions.stagingBuffer.buffer, collection->positions.buffer.buffer, 1, &positionsCopyRegion);
//    }
//    if (collection->normals.buffer)
//    {
//        vk::BufferCopy normalsCopyRegion = {};
//        normalsCopyRegion.size = collection->normals.count * 4 * sizeof(float);
//        commandBuffer.copyBuffer(collection->normals.stagingBuffer.buffer, collection->normals.buffer.buffer, 1, &normalsCopyRegion);
//    }
//    if (collection->tangents.buffer)
//    {
//        vk::BufferCopy tangentsCopyRegion = {};
//        tangentsCopyRegion.size = collection->tangents.count * 4 * sizeof(float);
//        commandBuffer.copyBuffer(collection->tangents.stagingBuffer.buffer, collection->tangents.buffer.buffer, 1, &tangentsCopyRegion);
//    }
//    if (collection->texcoords_0.buffer)
//    {
//        vk::BufferCopy texcoords_0CopyRegion = {};
//        texcoords_0CopyRegion.size = collection->texcoords_0.count * 2 * sizeof(float);
//        commandBuffer.copyBuffer(collection->texcoords_0.stagingBuffer.buffer, collection->texcoords_0.buffer.buffer, 1, &texcoords_0CopyRegion);
//    }
//    if (collection->colors_0.buffer)
//    {
//        vk::BufferCopy colors_0CopyRegion = {};
//        colors_0CopyRegion.size = collection->colors_0.count * 4 * sizeof(float);
//        commandBuffer.copyBuffer(collection->colors_0.stagingBuffer.buffer, collection->colors_0.buffer.buffer, 1, &colors_0CopyRegion);
//    }
//
//    commandBuffer.end();
//
//    vk::SubmitInfo submitInfo = {};
//    submitInfo.sType = vk::StructureType::eSubmitInfo;
//    submitInfo.commandBufferCount = 1;
//    submitInfo.pCommandBuffers = &commandBuffer;
//
//    (**_queue).submit(vk::ArrayProxy<vk::SubmitInfo>(submitInfo));
//    (**_queue).waitIdle();
//
//    (**_device).freeCommandBuffers(**_commandPool, 1, &commandBuffer);
//}
//
//void VulkanScene::copyIndicesData(uint32_t* destination, const std::vector<uint32_t>& indicesVector, size_t& offset)
//{
//    destination += offset;
//    memcpy(destination, indicesVector.data(), sizeof(uint32_t) * indicesVector.size());
//
//    offset += indicesVector.size();
//}
//
//template <typename type, typename attributeType, unsigned componentCount>
//void VulkanScene::copyAttributeData(type* destination, const std::vector<glm::vec<componentCount, attributeType>>& attributeVector, size_t& offset)
//{
//    destination += offset;
//
//    unsigned realCount = 0;
//    switch (componentCount)
//    {
//    case 1:
//        realCount = 1;
//        break;
//    case 2:
//        realCount = 2;
//        break;
//    case 3:
//        [[fallthrough]];
//    case 4:
//        realCount = 4;
//        break;
//    default:
//        break;
//    }
//
//    size_t dataPointer = 0;
//
//    for (const auto& element : attributeVector)
//    {
//        for (int i = 0; i < componentCount; i++)
//        {
//            destination[dataPointer] = element[i];
//            dataPointer++;
//        }
//        for (int i = 0; i < (realCount - componentCount); i++)
//        {
//            dataPointer++;
//        }
//    }
//
//    offset += attributeVector.size() * realCount;
//}
//
//void VulkanScene::createCollectionCommandBuffers(unsigned int framesInFlight)
//{
//    for (auto& categoryCollection : _collectionMap)
//    {
//        vk::CommandBufferAllocateInfo commandBufferInfo = {};
//        commandBufferInfo.sType = vk::StructureType::eCommandBufferAllocateInfo;
//        commandBufferInfo.level = vk::CommandBufferLevel::ePrimary;
//        commandBufferInfo.commandPool = **_commandPool;
//        commandBufferInfo.commandBufferCount = framesInFlight;
//
//        categoryCollection.second->commandBuffers = (**_device).allocateCommandBuffers(commandBufferInfo);
//    }
//}
//
//std::vector<vk::CommandBuffer> VulkanScene::recordFrameCommandBuffers(unsigned int frame, const vk::raii::Framebuffer& framebuffer, const unsigned int viewportWidth, const unsigned int viewportHeight)
//{
//    std::vector<vk::CommandBuffer> vector;
//
//    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)viewportWidth / (float)viewportHeight, 0.1f, 100.0f);
//
//    glm::vec3 cameraPos = glm::vec3(0.9f, 1.1f, 3.0f);
//    glm::vec3 cameraFront = glm::vec3(-0.2f, -0.2f, -1.0f);
//    glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
//    
//    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
//
//    ShaderStructures::PushConstants constants = { .mvpMatrix = projection * view };
//
//    for (auto& categoryCollection : _collectionMap)
//    {
//        vk::CommandBuffer& buffer = categoryCollection.second->commandBuffers[frame];
//        buffer.reset();
//
//        vk::CommandBufferBeginInfo bufferBeginInfo = {};
//        bufferBeginInfo.sType = vk::StructureType::eCommandBufferBeginInfo;
//
//        buffer.begin(bufferBeginInfo);
//
//        vk::RenderPassBeginInfo renderPassBeginInfo = {};
//        renderPassBeginInfo.sType = vk::StructureType::eRenderPassBeginInfo;
//        renderPassBeginInfo.renderPass = **_renderPass;
//        renderPassBeginInfo.framebuffer = framebuffer;
//        renderPassBeginInfo.renderArea.offset = vk::Offset2D(0, 0);
//        renderPassBeginInfo.renderArea.extent = vk::Extent2D(viewportWidth, viewportHeight);
//
//        vk::ClearValue clearValue(vk::ClearColorValue(0.2f, 0.2f, 0.2f, 1.0f));
//
//        renderPassBeginInfo.clearValueCount = 1;
//        renderPassBeginInfo.pClearValues = &clearValue;
//
//        buffer.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
//
//        buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, categoryCollection.second->materialPipeline.pipeline);
//
//        vk::DeviceSize offsets[] = { 0 };
//        buffer.bindIndexBuffer(categoryCollection.second->indices.buffer.buffer, offsets[0], vk::IndexType::eUint32);
//
//        vk::Viewport viewport;
//        viewport.x = 0.0f;
//        viewport.y = 0.0f;
//        viewport.width = viewportWidth;
//        viewport.height = viewportHeight;
//        viewport.minDepth = 0.0f;
//        viewport.maxDepth = 1.0f;
//
//        buffer.setViewport(0, 1, &viewport);
//
//        vk::Rect2D scissor;
//        scissor.offset = vk::Offset2D(0, 0);
//        scissor.extent = vk::Extent2D(viewportWidth, viewportHeight);
//
//        buffer.setScissor(0, 1, &scissor);
//
//        for (const auto& drawable : categoryCollection.second->drawables)
//        {
//            constants.mvpMatrix = projection * view * drawable.modelMatrix;
//
//            constants.addresses.positions = drawable.addresses.positions;
//            constants.addresses.normals = drawable.addresses.normals;
//            constants.addresses.tangents = drawable.addresses.tangents;
//            constants.addresses.texcoords_0 = drawable.addresses.texcoords_0;
//            constants.addresses.colors_0 = drawable.addresses.colors_0;
//
//            buffer.pushConstants(categoryCollection.second->materialPipeline.layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(constants), &constants);
//            buffer.drawIndexed(drawable.indexCount, 1, drawable.firstIndex, 0, 0);
//        }
//
//        buffer.endRenderPass();
//
//        buffer.end();
//
//        vector.emplace_back(buffer);
//    }
//    
//    return vector;
//}
