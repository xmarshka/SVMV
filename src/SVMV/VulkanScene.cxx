#include <SVMV/VulkanScene.hxx>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

using namespace SVMV;

namespace {
    std::unordered_map<std::shared_ptr<Primitive>, VulkanDrawable> primitiveDrawableMap;
}

VulkanScene::VulkanScene() : _allocator(nullptr)
{
}

VulkanScene::VulkanScene(vk::PhysicalDevice physicalDevice, vk::Instance instance, vk::Device device, vk::CommandPool commandPool, vk::Queue queue)
    : _device(device), _commandPool(commandPool), _queue(queue)
{
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = physicalDevice;
    allocatorInfo.device = device;
    allocatorInfo.instance = instance;
    allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

    vmaCreateAllocator(&allocatorInfo, &_allocator);
}

VulkanScene::~VulkanScene()
{
    /*for (auto& pair : _collectionMap)
    {
        if (pair.second.materialPipeline != nullptr)
        {
            _device.destroyPipeline(pair.second.graphicsPipeline);
            _device.destroyPipelineLayout(pair.second.layout);
        }
    }*/

    _collectionMap.clear();

    if (_allocator)
    {
        vmaDestroyAllocator(_allocator);
    }
}

void VulkanScene::setScene(std::shared_ptr<Scene> scene, vk::RenderPass renderPass, vkb::Swapchain swapchain, unsigned int framesInFlight)
{
    primitiveDrawableMap.clear();
    _renderPass = renderPass;

    divideSceneIntoCategories(scene, scene->root);

    loadSceneToGPUMemory(scene, swapchain, framesInFlight);

    createCollectionCommandBuffers(framesInFlight);
}

void VulkanScene::divideSceneIntoCategories(std::shared_ptr<Scene> scene, std::shared_ptr<Node> rootNode)
{
    if (rootNode->mesh)
    {
        for (const auto& primitive : rootNode->mesh->primitives)
        {
            VulkanDrawable drawable(rootNode->transform);

            // TODO: change this to not force GLTFPBR
            if (_collectionMap.find(MaterialName::GLTFPBR) == _collectionMap.end())
            {
                _collectionMap[MaterialName::GLTFPBR] = VulkanDrawableCollection();
            }

            VulkanDrawableCollection& collection = _collectionMap[MaterialName::GLTFPBR];

            if (primitiveDrawableMap.find(primitive) != primitiveDrawableMap.end()) // this primitive is already in the collection and has at least one associated drawable
            {
                drawable.firstIndex = primitiveDrawableMap[primitive].firstIndex;
                drawable.indexCount = primitiveDrawableMap[primitive].indexCount;
                drawable.vertexCount = primitiveDrawableMap[primitive].vertexCount;
            }
            else
            {
                collection.sources.emplace_back(primitive);
                drawable.firstIndex = collection.totalIndexCount;
                drawable.indexCount = primitive->indices.size();
                drawable.vertexCount = primitive->positions.size();

                primitiveDrawableMap[primitive] = drawable;

                collection.totalVertexCount += drawable.vertexCount;
                collection.totalIndexCount += drawable.indexCount;
                collection.totalIndexSize += drawable.indexCount * sizeof(primitive->indices[0]);
            }

            collection.drawables.emplace_back(drawable);
        }
    }

    for (const auto& childNode : rootNode->children)
    {
        divideSceneIntoCategories(scene, childNode);
    }
}

void VulkanScene::loadSceneToGPUMemory(std::shared_ptr<Scene> scene, vkb::Swapchain swapchain, unsigned int framesInFlight)
{
    vk::CommandBufferAllocateInfo commandBufferInfo = {};
    commandBufferInfo.sType = vk::StructureType::eCommandBufferAllocateInfo;
    commandBufferInfo.level = vk::CommandBufferLevel::ePrimary;
    commandBufferInfo.commandPool = _commandPool;
    commandBufferInfo.commandBufferCount = 1;
    vk::CommandBuffer commandBuffer = _device.allocateCommandBuffers(commandBufferInfo)[0];

    for (auto& materialCollection : _collectionMap)
    {
        VulkanDrawableCollection& collection = materialCollection.second;

        auto bufferUsage = vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eShaderDeviceAddress;
        collection.vertexBuffer.create(_allocator, collection.totalVertexCount * sizeof(VulkanVertex), bufferUsage, VMA_MEMORY_USAGE_GPU_ONLY);

        VulkanBuffer stagingVertexBuffer;
        stagingVertexBuffer.create(_allocator, collection.totalVertexCount * sizeof(VulkanVertex), vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_ONLY);

        loadCollectionVerticesToBuffer(collection, stagingVertexBuffer);
        
        vk::CommandBufferBeginInfo commandBufferBeginInfo = {};
        commandBufferBeginInfo.sType = vk::StructureType::eCommandBufferBeginInfo;
        commandBufferBeginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

        commandBuffer.begin(commandBufferBeginInfo);

        VulkanBuffer stagingIndexBuffer;
        stagingIndexBuffer.create(_allocator, collection.totalIndexSize, vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_ONLY);
        collection.indexBuffer.create(_allocator, collection.totalIndexSize, bufferUsage, VMA_MEMORY_USAGE_GPU_ONLY);

        vk::BufferCopy indexCopyRegion = {};
        indexCopyRegion.size = collection.totalIndexSize;

        vk::BufferCopy vertexCopyRegion = {};
        vertexCopyRegion.size = collection.totalVertexCount * sizeof(VulkanVertex);

        commandBuffer.copyBuffer(stagingIndexBuffer.buffer, collection.indexBuffer.buffer, 1, &indexCopyRegion);

        commandBuffer.copyBuffer(stagingVertexBuffer.buffer, collection.vertexBuffer.buffer, 1, &vertexCopyRegion);

        commandBuffer.end();

        vk::SubmitInfo submitInfo = {};
        submitInfo.sType = vk::StructureType::eSubmitInfo;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        _queue.submit(vk::ArrayProxy<vk::SubmitInfo>(submitInfo));
        _queue.waitIdle();

        _device.freeCommandBuffers(_commandPool, 1, &commandBuffer);
    }
}

void VulkanScene::loadCollectionVerticesToBuffer(VulkanDrawableCollection& collection, VulkanBuffer& stagingBuffer)
{
    VulkanVertex* data = reinterpret_cast<VulkanVertex*>(stagingBuffer.allocation->GetMappedData());

    unsigned vertexIndex = 0;
    for (const auto& source : collection.sources)
    {
        for (unsigned i = 0; i < source->positions.size(); i++)
        {
            data[vertexIndex].position = source->positions[i];
            data[vertexIndex].normal = source->normals[i];

            if (source->tangens.size() > 0)
            {
                data[vertexIndex].tangent = source->tangens[i];
            }

            if (source->texcoords_0.size() > 0)
            {
                data[vertexIndex].texcoordX = source->texcoords_0[i][0];
                data[vertexIndex].texcoordY = source->texcoords_0[i][1];
            }

            if (source->colors_0.size() > 0)
            {
                data[vertexIndex].color = source->colors_0[i];
            }
            else
            {
                data[vertexIndex].color = glm::fvec4(1.0f);
            }

            vertexIndex++;
        }
    }
}

void VulkanScene::createCollectionCommandBuffers(unsigned int framesInFlight)
{
    for (auto& categoryCollection : _collectionMap)
    {
        vk::CommandBufferAllocateInfo commandBufferInfo = {};
        commandBufferInfo.sType = vk::StructureType::eCommandBufferAllocateInfo;
        commandBufferInfo.level = vk::CommandBufferLevel::ePrimary;
        commandBufferInfo.commandPool = _commandPool;
        commandBufferInfo.commandBufferCount = framesInFlight;

        categoryCollection.second.commandBuffers = _device.allocateCommandBuffers(commandBufferInfo);
    }
}

std::vector<vk::CommandBuffer> VulkanScene::recordFrameCommandBuffers(unsigned int frame, vk::Framebuffer framebuffer, const unsigned int viewportWidth, const unsigned int viewportHeight)
{
    std::vector<vk::CommandBuffer> vector;

    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)viewportWidth / (float)viewportHeight, 0.1f, 100.0f);

    glm::vec3 cameraPos = glm::vec3(0.9f, 1.1f, 3.0f);
    glm::vec3 cameraFront = glm::vec3(-0.2f, -0.2f, -1.0f);
    glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
    
    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

    ShaderStructures::PushConstants constants = { .mvpMatrix = projection * view };

    for (auto& categoryCollection : _collectionMap)
    {
        vk::BufferDeviceAddressInfo deviceAddressInfo = {};
        deviceAddressInfo.sType = vk::StructureType::eBufferDeviceAddressInfo;
        deviceAddressInfo.buffer = categoryCollection.second.vertexBuffer.buffer;

        constants.vertexBufferAddress = _device.getBufferAddress(deviceAddressInfo);

        //deviceAddressInfo.buffer = categoryCollection.second.attributeBuffers[2].buffer;
        //constants.colorsAddress = _device.getBufferAddress(deviceAddressInfo);

        vk::CommandBuffer& buffer = categoryCollection.second.commandBuffers[frame];
        buffer.reset();

        vk::CommandBufferBeginInfo bufferBeginInfo = {};
        bufferBeginInfo.sType = vk::StructureType::eCommandBufferBeginInfo;

        buffer.begin(bufferBeginInfo);

        vk::RenderPassBeginInfo renderPassBeginInfo = {};
        renderPassBeginInfo.sType = vk::StructureType::eRenderPassBeginInfo;
        renderPassBeginInfo.renderPass = _renderPass;
        renderPassBeginInfo.framebuffer = framebuffer;
        renderPassBeginInfo.renderArea.offset = vk::Offset2D(0, 0);
        renderPassBeginInfo.renderArea.extent = vk::Extent2D(viewportWidth, viewportHeight);

        vk::ClearValue clearValue(vk::ClearColorValue(0.2f, 0.2f, 0.2f, 1.0f));

        renderPassBeginInfo.clearValueCount = 1;
        renderPassBeginInfo.pClearValues = &clearValue;

        buffer.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);

        buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, categoryCollection.second.materialPipeline.pipeline);

        vk::DeviceSize offsets[] = { 0 };
        buffer.bindIndexBuffer(categoryCollection.second.indexBuffer.buffer, offsets[0], vk::IndexType::eUint32);

        vk::Viewport viewport;
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = viewportWidth;
        viewport.height = viewportHeight;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        buffer.setViewport(0, 1, &viewport);

        vk::Rect2D scissor;
        scissor.offset = vk::Offset2D(0, 0);
        scissor.extent = vk::Extent2D(viewportWidth, viewportHeight);

        buffer.setScissor(0, 1, &scissor);

        for (const auto& drawable : categoryCollection.second.drawables)
        {
            constants.mvpMatrix = projection * view * drawable.modelMatrix;

            buffer.pushConstants(categoryCollection.second.materialPipeline.layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(constants), &constants);
            buffer.drawIndexed(drawable.indexCount, 1, drawable.firstIndex, 0, 0);
        }

        buffer.endRenderPass();

        buffer.end();

        vector.emplace_back(buffer);
    }
    
    return vector;
}

