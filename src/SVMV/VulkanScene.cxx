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
    for (auto& pair : _collectionMap)
    {
        if (pair.second.graphicsPipeline != nullptr)
        {
            _device.destroyPipeline(pair.second.graphicsPipeline);
            _device.destroyPipelineLayout(pair.second.layout);
        }
    }

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

    createCollectionPipelines(renderPass, swapchain);
    createCollectionCommandBuffers(framesInFlight);
}

void VulkanScene::divideSceneIntoCategories(std::shared_ptr<Scene> scene, std::shared_ptr<Node> rootNode)
{
    if (rootNode->mesh)
    {
        for (const auto& primitive : rootNode->mesh->primitives)
        {
            VulkanDrawableCategory category = {};

            if (primitive->material == nullptr)
            {
                category.shaderCategory = VulkanDrawableCategory::ShaderCategory::FLAT;
            }

            category.vertexAttributeCategory = getVertexAttributeCategory(primitive);

            if (_collectionMap.find(category) == _collectionMap.end())
            {
                VulkanDrawableCollection collection;
                _collectionMap[category] = collection;
            }

            auto& collection = _collectionMap[category];

            VulkanDrawable drawable(rootNode->transform);

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
                drawable.vertexCount = primitive->attributes[0]->count;

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

unsigned char VulkanScene::getVertexAttributeCategory(const std::shared_ptr<Primitive> primitive)
{
    unsigned char attributeField = 0;

    for (const auto& attribute : primitive->attributes)
    {
        attributeField = attributeField | (1 << static_cast<uint32_t>(attribute->type));
    }

    return attributeField;
}

void VulkanScene::loadSceneToGPUMemory(std::shared_ptr<Scene> scene, vkb::Swapchain swapchain, unsigned int framesInFlight)
{
    VulkanBuffer indexStagingBuffer;
    std::vector<VulkanBuffer> stagingBuffers;
    stagingBuffers.reserve(static_cast<size_t>(Attribute::AttributeType::_COUNT));
    std::vector<vk::BufferCopy> stagingBufferCopyRegions;
    stagingBufferCopyRegions.reserve(static_cast<size_t>(Attribute::AttributeType::_COUNT));

    vk::CommandBufferAllocateInfo commandBufferInfo = {};
    commandBufferInfo.sType = vk::StructureType::eCommandBufferAllocateInfo;
    commandBufferInfo.level = vk::CommandBufferLevel::ePrimary;
    commandBufferInfo.commandPool = _commandPool;
    commandBufferInfo.commandBufferCount = 1;
    vk::CommandBuffer commandBuffer = _device.allocateCommandBuffers(commandBufferInfo)[0];

    for (auto& categoryCollection : _collectionMap)
    {
        const VulkanDrawableCategory& category = categoryCollection.first;
        VulkanDrawableCollection& collection = categoryCollection.second;
        
        const auto& collectionAttributes = collection.sources[0]->attributes; // only used for info about attributes, since they are all the same for a collection
        collection.attributeBuffers.reserve(static_cast<size_t>(Attribute::AttributeType::_COUNT));

        // create buffer for indices
        auto bufferUsage = vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst;
        collection.indices.create(_allocator, collection.totalIndexSize, bufferUsage, VMA_MEMORY_USAGE_GPU_ONLY);
        indexStagingBuffer.create(_allocator, collection.totalIndexSize, vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_ONLY);

        int attributeCounter = 0;
        for (uint8_t i = 0; i < static_cast<uint8_t>(Attribute::AttributeType::_COUNT); i++)
        {
            if (category.vertexAttributeCategory & (1 << i))
            {
                size_t size = collection.totalVertexCount * (collectionAttributes[attributeCounter]->components + collectionAttributes[attributeCounter]->padding) * collectionAttributes[attributeCounter]->componentSize;

                bufferUsage = vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eShaderDeviceAddress;

                collection.attributeBuffers.push_back(VulkanBuffer());
                collection.attributeBuffers[attributeCounter].create(_allocator, size, bufferUsage, VMA_MEMORY_USAGE_GPU_ONLY);

                stagingBuffers.push_back(VulkanBuffer());
                stagingBuffers[attributeCounter].create(_allocator, size, vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_ONLY);

                attributeCounter++;
            }
        }

        int indexOffset = 0;
        int vertexOffset = 0;

        // copy data to staging buffers
        for (int i = 0; i < collection.sources.size(); i++)
        {
            void* data = indexStagingBuffer.allocation->GetMappedData();
            memcpy(reinterpret_cast<uint8_t*>(data) + (indexOffset * sizeof(collection.sources[i]->indices[0])),
                collection.sources[i]->indices.data(),
                collection.sources[i]->indices.size() * sizeof(collection.sources[i]->indices[0])
            );

            for (uint8_t j = 0; j < stagingBuffers.size(); j++)
            {
                data = stagingBuffers[j].allocation->GetMappedData();
                memcpy(
                    reinterpret_cast<uint8_t*>(data) + (vertexOffset * (collectionAttributes[j]->components + collectionAttributes[j]->padding) * collectionAttributes[j]->componentSize),
                    collection.sources[i]->attributes[j]->data.data(),
                    collection.sources[i]->attributes[j]->count * (collectionAttributes[j]->components + collectionAttributes[j]->padding) * collectionAttributes[j]->componentSize
                );
            }

            indexOffset += collection.drawables[i].indexCount;
            vertexOffset += collection.drawables[i].vertexCount;
        }

        vk::CommandBufferBeginInfo commandBufferBeginInfo = {};
        commandBufferBeginInfo.sType = vk::StructureType::eCommandBufferBeginInfo;
        commandBufferBeginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

        commandBuffer.begin(commandBufferBeginInfo);

        vk::BufferCopy indexCopyRegion = {};
        indexCopyRegion.size = collection.totalIndexSize;

        commandBuffer.copyBuffer(indexStagingBuffer.buffer, collection.indices.buffer, 1, &indexCopyRegion);

        for (uint8_t i = 0; i < collection.attributeBuffers.size(); i++)
        {
            stagingBufferCopyRegions.emplace_back(vk::BufferCopy(0, 0, collection.attributeBuffers[i].info.size));
            commandBuffer.copyBuffer(stagingBuffers[i].buffer, collection.attributeBuffers[i].buffer, 1, &stagingBufferCopyRegions[i]);
        }

        commandBuffer.end();

        vk::SubmitInfo submitInfo = {};
        submitInfo.sType = vk::StructureType::eSubmitInfo;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        _queue.submit(vk::ArrayProxy<vk::SubmitInfo>(submitInfo));
        _queue.waitIdle();

        _device.freeCommandBuffers(_commandPool, 1, &commandBuffer);
    }

    // all the combined vertices of all primitives are divided into streams for each attribute
    // and the shader gets, as a push constant, all the addresses to the buffers (every shader gets every
    // address, they only use the ones they need, and the rest are null pointers)
}

void VulkanScene::createCollectionPipelines(vk::RenderPass renderPass, vkb::Swapchain swapchain)
{
    for (auto& pair : _collectionMap)
    {
        if (pair.first.shaderCategory == VulkanDrawableCategory::ShaderCategory::FLAT)
        {
            VulkanShader vertexShader(_device, VulkanShader::ShaderType::VERTEX, "shader.vert");
            VulkanShader fragmentShader(_device, VulkanShader::ShaderType::FRAGMENT, "shader.frag");

            vk::PipelineShaderStageCreateInfo shaderStages[2];

            shaderStages[0].sType = vk::StructureType::ePipelineShaderStageCreateInfo;
            shaderStages[0].stage = vk::ShaderStageFlagBits::eVertex;
            shaderStages[0].module = vertexShader.shader;
            shaderStages[0].pName = "main";

            shaderStages[1].sType = vk::StructureType::ePipelineShaderStageCreateInfo;
            shaderStages[1].stage = vk::ShaderStageFlagBits::eFragment;
            shaderStages[1].module = fragmentShader.shader;
            shaderStages[1].pName = "main";

            vk::PipelineVertexInputStateCreateInfo vertexInputStateInfo = {};
            vertexInputStateInfo.sType = vk::StructureType::ePipelineVertexInputStateCreateInfo;
            vertexInputStateInfo.vertexBindingDescriptionCount = 0;
            vertexInputStateInfo.vertexAttributeDescriptionCount = 0;

            vk::PipelineInputAssemblyStateCreateInfo assemblyStateInfo = {};
            assemblyStateInfo.sType = vk::StructureType::ePipelineInputAssemblyStateCreateInfo;
            assemblyStateInfo.topology = vk::PrimitiveTopology::eTriangleList;
            assemblyStateInfo.primitiveRestartEnable = vk::False;

            vk::Viewport viewport;
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = swapchain.extent.width;
            viewport.height = swapchain.extent.height;
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;

            vk::Rect2D scissor;
            scissor.offset = vk::Offset2D(0, 0);
            scissor.extent = vk::Extent2D(swapchain.extent.width, swapchain.extent.height);

            std::vector<vk::DynamicState> dynamicStates = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };
            vk::PipelineDynamicStateCreateInfo dynamicStateInfo = {};
            dynamicStateInfo.sType = vk::StructureType::ePipelineDynamicStateCreateInfo;
            dynamicStateInfo.dynamicStateCount = dynamicStates.size();
            dynamicStateInfo.pDynamicStates = dynamicStates.data();

            vk::PipelineViewportStateCreateInfo viewportStateInfo = {};
            viewportStateInfo.sType = vk::StructureType::ePipelineViewportStateCreateInfo;
            viewportStateInfo.viewportCount = 1;
            viewportStateInfo.pViewports = &viewport;
            viewportStateInfo.scissorCount = 1;
            viewportStateInfo.pScissors = &scissor;

            vk::PipelineRasterizationStateCreateInfo rasterizationStateInfo = {};
            rasterizationStateInfo.sType = vk::StructureType::ePipelineRasterizationStateCreateInfo;
            rasterizationStateInfo.depthClampEnable = vk::False;
            rasterizationStateInfo.rasterizerDiscardEnable = vk::False;
            rasterizationStateInfo.polygonMode = vk::PolygonMode::eFill;
            rasterizationStateInfo.lineWidth = 1.0f;
            rasterizationStateInfo.cullMode = vk::CullModeFlagBits::eBack;
            rasterizationStateInfo.frontFace = vk::FrontFace::eCounterClockwise;
            rasterizationStateInfo.depthBiasEnable = vk::False;

            vk::PipelineMultisampleStateCreateInfo multisamplingInfo = {};
            multisamplingInfo.sType = vk::StructureType::ePipelineMultisampleStateCreateInfo;
            multisamplingInfo.sampleShadingEnable = vk::False;
            multisamplingInfo.rasterizationSamples = vk::SampleCountFlagBits::e1;

            vk::PipelineColorBlendAttachmentState colorBlendAttachmentState = {};
            colorBlendAttachmentState.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
            colorBlendAttachmentState.blendEnable = vk::False;

            vk::PipelineColorBlendStateCreateInfo colorBlendStateInfo = {};
            colorBlendStateInfo.sType = vk::StructureType::ePipelineColorBlendStateCreateInfo;
            colorBlendStateInfo.logicOpEnable = vk::False;
            colorBlendStateInfo.attachmentCount = 1;
            colorBlendStateInfo.pAttachments = &colorBlendAttachmentState;

            vk::PushConstantRange pushConstantRange = {};
            pushConstantRange.offset = 0;
            pushConstantRange.size = sizeof(PushConstants);
            pushConstantRange.stageFlags = vk::ShaderStageFlagBits::eVertex;

            vk::PipelineLayoutCreateInfo pipelineLayoutInfo = {};
            pipelineLayoutInfo.sType = vk::StructureType::ePipelineLayoutCreateInfo;
            pipelineLayoutInfo.pushConstantRangeCount = 1;
            pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

            pair.second.layout = _device.createPipelineLayout(pipelineLayoutInfo);

            vk::GraphicsPipelineCreateInfo info = {};
            info.sType = vk::StructureType::eGraphicsPipelineCreateInfo;
            info.stageCount = 2;
            info.pStages = shaderStages;
            info.pVertexInputState = &vertexInputStateInfo;
            info.pInputAssemblyState = &assemblyStateInfo;
            info.pViewportState = &viewportStateInfo;
            info.pRasterizationState = &rasterizationStateInfo;
            info.pMultisampleState = &multisamplingInfo;
            info.pColorBlendState = &colorBlendStateInfo;
            info.pDynamicState = &dynamicStateInfo;
            info.layout = pair.second.layout;
            info.renderPass = _renderPass;
            info.subpass = 0;

            auto result = _device.createGraphicsPipeline(nullptr, info);

            if (result.result != vk::Result::eSuccess)
            {
                throw std::runtime_error("vulkan: failed to create graphics pipeline");
            }

            pair.second.graphicsPipeline = result.value;
        }
    }
}

void VulkanScene::createCollectionCommandBuffers(unsigned int framesInFlight)
{
    for (auto& pair : _collectionMap)
    {
        vk::CommandBufferAllocateInfo commandBufferInfo = {};
        commandBufferInfo.sType = vk::StructureType::eCommandBufferAllocateInfo;
        commandBufferInfo.level = vk::CommandBufferLevel::ePrimary;
        commandBufferInfo.commandPool = _commandPool;
        commandBufferInfo.commandBufferCount = framesInFlight;

        pair.second.commandBuffers = _device.allocateCommandBuffers(commandBufferInfo);
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

    PushConstants constants = { .mvpMatrix = projection * view };

    for (auto& pair : _collectionMap)
    {
        vk::BufferDeviceAddressInfo deviceAddressInfo = {};
        deviceAddressInfo.sType = vk::StructureType::eBufferDeviceAddressInfo;
        deviceAddressInfo.buffer = pair.second.attributeBuffers[0].buffer;

        constants.positionsAddress = _device.getBufferAddress(deviceAddressInfo);

        //deviceAddressInfo.buffer = pair.second.attributeBuffers[2].buffer;
        //constants.colorsAddress = _device.getBufferAddress(deviceAddressInfo);

        vk::CommandBuffer& buffer = pair.second.commandBuffers[frame];
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

        buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pair.second.graphicsPipeline);

        vk::DeviceSize offsets[] = { 0 };
        buffer.bindIndexBuffer(pair.second.indices.buffer, offsets[0], vk::IndexType::eUint32);

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

        for (const auto& drawable : pair.second.drawables)
        {
            constants.mvpMatrix = projection * view * drawable.modelMatrix;

            buffer.pushConstants(pair.second.layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(constants), &constants);
            buffer.drawIndexed(drawable.indexCount, 1, drawable.firstIndex, 0, 0);
        }

        buffer.endRenderPass();

        buffer.end();

        vector.emplace_back(buffer);
    }
    
    return vector;
}

