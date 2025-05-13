#include <SVMV/VulkanRenderer.hxx>

using namespace SVMV;

VulkanRenderer::VulkanRenderer(int width, int height, const std::string& name, unsigned framesInFlight, const GLFWwindowWrapper& window)
    : _framesInFlight(framesInFlight)
{
    _instance = _initilization.createInstance(_context, name, 1, 3);
    _messenger = _initilization.createDebugMessenger(_instance);

    _surface = window.createVulkanSurface(_instance);

    _physicalDevice = _initilization.createPhysicalDevice(_instance, std::vector<const char*>{ vk::KHRBufferDeviceAddressExtensionName }, _surface);
    _device = _initilization.createDevice(_physicalDevice);
    _swapchain = _initilization.createSwapchain(_device, _surface);
    _swapchainExtent = _initilization.getSwapchainExtent();
    _swapchainFormat = _initilization.getSwapchainFormat();

    _commandPool = _initilization.createCommandPool(_device);

    auto graphicsQueuePair = _initilization.createQueue(_device, vkb::QueueType::graphics);
    _graphicsQueue = std::move(graphicsQueuePair.first);
    _graphicsQueueIndex = graphicsQueuePair.second;

    auto presentQueuePair = _initilization.createQueue(_device, vkb::QueueType::present);
    _presentQueue = std::move(presentQueuePair.first);
    _presentQueueIndex = presentQueuePair.second;

    auto computeQueuePair = _initilization.createQueue(_device, vkb::QueueType::compute);
    _computeQueue = std::move(computeQueuePair.first);
    _computeQueueIndex = computeQueuePair.second;

    _descriptorAllocator = VulkanUtilities::DescriptorAllocator(&_device);
    _descriptorWriter = VulkanDescriptorWriter(&_device);
    _vmaAllocator = VulkanUtilities::VmaAllocatorWrapper(_instance, _physicalDevice, _device);
    _immediateSubmit = VulkanUtilities::ImmediateSubmit(&_device, &_graphicsQueue, _graphicsQueueIndex);

    createDepthBuffer();
    createRenderPass();

    _imageViews = _initilization.createSwapchainImageViews(_device);
    _framebuffers = _initilization.createFramebuffers(_device, _renderPass, _imageViews, _depthBuffer.getImageView());
    _imageReadySemaphores = _initilization.createSemaphores(_device, _framesInFlight);
    _renderCompleteSemaphores = _initilization.createSemaphores(_device, _framesInFlight);
    _inFlightFences = _initilization.createFences(_device, _framesInFlight);

    vk::CommandBufferAllocateInfo commandBufferAllocateInfo;
    commandBufferAllocateInfo.setLevel(vk::CommandBufferLevel::ePrimary);
    commandBufferAllocateInfo.setCommandPool(_commandPool);
    commandBufferAllocateInfo.setCommandBufferCount(_framesInFlight);

    _drawCommandBuffers = vk::raii::CommandBuffers(_device, commandBufferAllocateInfo);
    createGlobalDescriptorSets();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // IF using Docking Branch

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForVulkan(window.getWindow(), true);

    // initialize imgui vulkan objects

    vk::DescriptorPoolSize poolSizes[] = {
        { vk::DescriptorType::eSampler, 1000 },
        { vk::DescriptorType::eCombinedImageSampler, 1000 },
        { vk::DescriptorType::eSampledImage, 1000 },
        { vk::DescriptorType::eStorageImage, 1000 },
        { vk::DescriptorType::eUniformTexelBuffer, 1000 },
        { vk::DescriptorType::eStorageTexelBuffer, 1000 },
        { vk::DescriptorType::eUniformBuffer, 1000 },
        { vk::DescriptorType::eStorageBuffer, 1000 },
        { vk::DescriptorType::eUniformBufferDynamic, 1000 },
        { vk::DescriptorType::eStorageBufferDynamic, 1000 },
        { vk::DescriptorType::eInputAttachment, 1000 }
    };

    vk::DescriptorPoolCreateInfo info;
    info.setMaxSets(4096);
    info.setPoolSizes(poolSizes);
    info.setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet);

    _imguiDescriptorPool = vk::raii::DescriptorPool(_device, info);

    vk::AttachmentDescription attachmentDescription;
    attachmentDescription.setFormat(_swapchainFormat);
    attachmentDescription.setSamples(vk::SampleCountFlagBits::e1);
    attachmentDescription.setLoadOp(vk::AttachmentLoadOp::eLoad);
    attachmentDescription.setStoreOp(vk::AttachmentStoreOp::eStore);
    attachmentDescription.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare);
    attachmentDescription.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);
    attachmentDescription.setInitialLayout(vk::ImageLayout::eColorAttachmentOptimal);
    attachmentDescription.setFinalLayout(vk::ImageLayout::ePresentSrcKHR);

    vk::AttachmentReference attachmentReference;
    attachmentReference.setAttachment(0);
    attachmentReference.setLayout(vk::ImageLayout::eColorAttachmentOptimal);

    vk::SubpassDescription subpassDescription;
    subpassDescription.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics);
    subpassDescription.setColorAttachments(attachmentReference);

    vk::SubpassDependency subpassDependency;
    subpassDependency.setSrcSubpass(vk::SubpassExternal);
    subpassDependency.setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
    subpassDependency.setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
    subpassDependency.setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite);

    vk::RenderPassCreateInfo renderPassCreateInfo;
    renderPassCreateInfo.setAttachments(attachmentDescription);
    renderPassCreateInfo.setSubpasses(subpassDescription);
    renderPassCreateInfo.setDependencies(subpassDependency);

    _imguiRenderPass = vk::raii::RenderPass(_device, renderPassCreateInfo);

    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = *_instance;
    init_info.PhysicalDevice = *_physicalDevice;
    init_info.RenderPass = *_imguiRenderPass;
    init_info.Device = *_device;
    init_info.QueueFamily = _graphicsQueueIndex;
    init_info.Queue = *_graphicsQueue;
    init_info.DescriptorPool = *_imguiDescriptorPool;
    init_info.Subpass = 0;
    init_info.MinImageCount = framesInFlight;
    init_info.ImageCount = framesInFlight;
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.CheckVkResultFn = [](VkResult result) { if (result != VK_SUCCESS) throw std::runtime_error("imgui: error"); };
    ImGui_ImplVulkan_Init(&init_info);

    _imguiFramebuffers = _initilization.createFramebuffers(_device, _imguiRenderPass, _imageViews);

    _light = VulkanLight(
        glm::vec4(1.0f, 1.0f, 1.0f, 0.0f), glm::vec4(2.0f, 3.0f, 2.0f, 0.0f), &_device, _vmaAllocator.getAllocator(), _lightDescriptorSetLayout, &_descriptorAllocator, &_descriptorWriter, framesInFlight
    );
}

VulkanRenderer::~VulkanRenderer()
{
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void VulkanRenderer::draw()
{
    if (!_requestedScenePath.empty())
    {
        try
        {
            loadScene(Loader::loadScene(_requestedScenePath));
        }
        catch (...)
        {
            std::cout << "Error loading glTF file: " << _requestedScenePath << "; skipping model." << std::endl;
        }

        _requestedScenePath.clear();
    }

    // wait for this frame index's render to be finished
    vk::Result waitForFencesResult = _device.waitForFences(*_inFlightFences[_activeFrame], vk::True, UINT64_MAX);

    if (waitForFencesResult != vk::Result::eSuccess)
    {
        throw std::runtime_error("vulkan: failure waiting for fences");
    }

    // acquire an image for color output
    vk::AcquireNextImageInfoKHR acquireNextImageInfo;
    acquireNextImageInfo.setSwapchain(_swapchain);
    acquireNextImageInfo.setTimeout(UINT64_MAX);
    acquireNextImageInfo.setSemaphore(_imageReadySemaphores[_activeFrame]);
    acquireNextImageInfo.setDeviceMask(0b1);

    std::pair<vk::Result, uint32_t> acquireResult;

    try
    {
        acquireResult = _device.acquireNextImage2KHR(acquireNextImageInfo);
    }
    catch (vk::OutOfDateKHRError)
    {
        recreateSwapchain();
    }
    catch (...)
    {
        throw std::runtime_error("vulkan: failed to acquire next image");
    }

    _device.resetFences(*_inFlightFences[_activeFrame]);

    // set the current frame's camera matrix descriptor set
    ShaderStructures::GlobalUniformBuffer globalUniformBuffer;
    globalUniformBuffer.View = _viewMatrix;
    globalUniformBuffer.ViewProjection = _projectionMatrix * _viewMatrix;
    globalUniformBuffer.CameraPosition = glm::vec4(_cameraPosition.x, _cameraPosition.y, _cameraPosition.z, 0.0f);

    _globalDescriptorSetBuffers[_activeFrame].setData(&globalUniformBuffer, sizeof(ShaderStructures::GlobalUniformBuffer));

    _descriptorWriter.writeBuffer(_globalDescriptorSets[_activeFrame], _globalDescriptorSetBuffers[_activeFrame], 0, 0, sizeof(ShaderStructures::GlobalUniformBuffer), vk::DescriptorType::eUniformBuffer);

    // update this frame's light descriptor set
    _light.updateLightDescriptor(_activeFrame);

    // record draw command buffers
    _drawCommandBuffers[_activeFrame].reset();
    recordDrawCommands(_activeFrame, _framebuffers[acquireResult.second], _imguiFramebuffers[acquireResult.second]);

    // submit command buffer to graphics queue for execution
    vk::SubmitInfo submitInfo;
    submitInfo.setWaitSemaphores(*(_imageReadySemaphores[_activeFrame]));
    vk::PipelineStageFlags waitDstStageFlags = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    submitInfo.setWaitDstStageMask(waitDstStageFlags);
    submitInfo.setCommandBuffers(*(_drawCommandBuffers[_activeFrame]));
    submitInfo.setSignalSemaphores(*(_renderCompleteSemaphores[_activeFrame]));

    _graphicsQueue.submit(submitInfo, *_inFlightFences[_activeFrame]);

    // present the frame to the screen
    vk::PresentInfoKHR presentInfo;
    presentInfo.setWaitSemaphores(*(_renderCompleteSemaphores[_activeFrame]));
    presentInfo.setSwapchains(*(_swapchain));
    presentInfo.setImageIndices(acquireResult.second);

    try
    {
        _presentQueue.presentKHR(presentInfo);
    }
    catch (vk::OutOfDateKHRError)
    {
        recreateSwapchain();
    }
    catch (...)
    {
        throw std::runtime_error("vulkan: failed to acquire next image");
    }

    // increment active frame index
    _activeFrame = (_activeFrame + 1) % _framesInFlight;
}

void VulkanRenderer::loadScene(std::shared_ptr<Scene> scene)
{
    _device.waitIdle(); // wait here for all the frames to finish rendering and presenting

    _scene = VulkanScene();

    preprocessScene(scene);
    generateDrawablesFromScene(scene->root, scene->root->transform);
    copyStagingBuffersToGPUBuffers();
}

void VulkanRenderer::setCamera(glm::vec3 position, glm::vec3 lookDirection, glm::vec3 upDirection, float fieldOfView)
{
    _projectionMatrix = glm::perspective(glm::radians(fieldOfView), (float)_swapchainExtent.width / (float)_swapchainExtent.height, 0.01f, 100.0f);
    _projectionMatrix[1][1] *= -1;

    _viewMatrix = glm::lookAt(position, position + lookDirection, upDirection);

    _cameraPosition = position;
}

const vk::Device VulkanRenderer::getDevice() const noexcept
{
    return (*_device);
}

void VulkanRenderer::recordDrawCommands(int activeFrame, const vk::raii::Framebuffer& framebuffer, const vk::raii::Framebuffer& imguiFramebuffer)
{
    ShaderStructures::PushConstants constants;

    _drawCommandBuffers[activeFrame].begin(vk::CommandBufferBeginInfo());

    vk::RenderPassBeginInfo renderPassBeginInfo;
    renderPassBeginInfo.setRenderPass(_renderPass);
    renderPassBeginInfo.setFramebuffer(framebuffer);
    renderPassBeginInfo.setRenderArea(vk::Rect2D(vk::Offset2D(0, 0), _swapchainExtent));

    glm::vec3 backgroundColor = glm::vec3(_light.lightData.ambient.x, _light.lightData.ambient.y, _light.lightData.ambient.z) * 0.3f;

    vk::ClearValue clearValues[2] = { vk::ClearColorValue(backgroundColor.x, backgroundColor.y, backgroundColor.z, 1.0f), vk::ClearDepthStencilValue(1.0f, 0.0f) };
    renderPassBeginInfo.setClearValues(clearValues);

    _drawCommandBuffers[activeFrame].beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);

    vk::Viewport viewport(0.0f, 0.0f, _swapchainExtent.width, _swapchainExtent.height, 0.0f, 1.0f);
    _drawCommandBuffers[activeFrame].setViewport(0, viewport);

    vk::Rect2D scissor(vk::Offset2D(0, 0), _swapchainExtent);
    _drawCommandBuffers[activeFrame].setScissor(0, scissor);

    for (const auto& context : _scene.contexts)
    {
        _drawCommandBuffers[activeFrame].bindPipeline(vk::PipelineBindPoint::eGraphics, *context.second.pipeline);

        _drawCommandBuffers[activeFrame].bindIndexBuffer(_scene.indexGPUBuffer.getBuffer(), vk::DeviceSize(0), vk::IndexType::eUint32);

        _drawCommandBuffers[activeFrame].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *context.second.pipelineLayout, 0, *_globalDescriptorSets[activeFrame], nullptr);

        _drawCommandBuffers[activeFrame].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *context.second.pipelineLayout, 1, **_light.getDescriptorSet(activeFrame), nullptr);

        for (const auto& drawable : context.second.drawables)
        {
            _drawCommandBuffers[activeFrame].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *context.second.pipelineLayout, 2, drawable.descriptorSet, nullptr);

            constants.positions = drawable.attributeAddresses.positions;
            constants.normals = drawable.attributeAddresses.normals;
            constants.tangents = drawable.attributeAddresses.tangents;
            constants.texcoords_0 = drawable.attributeAddresses.texcoords_0;
            constants.colors_0 = drawable.attributeAddresses.colors_0;
            constants.modelMatrix = drawable.modelMatrixAddress;
            constants.normalMatrix = drawable.normalMatrixAddress;

            _drawCommandBuffers[activeFrame].pushConstants<ShaderStructures::PushConstants>(*context.second.pipelineLayout, vk::ShaderStageFlagBits::eVertex, 0, constants);
            _drawCommandBuffers[activeFrame].drawIndexed(drawable.indexCount, 1, drawable.firstIndex, 0, 0);
        }
    }

    _drawCommandBuffers[activeFrame].endRenderPass();

    // imgui render pass

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    const float panelWidth = _swapchainExtent.width * 0.2f; // 20% of screen width

    // imgui layout for the left hand panel
    {
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(panelWidth, _swapchainExtent.height));

        ImGuiWindowFlags windowFlags =
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoTitleBar;

        ImGui::Begin("Left Hand Panel", nullptr, windowFlags);
        {
            ImGui::SeparatorText("File:");
            if (ImGui::Button("Select file")) {
                IGFD::FileDialogConfig config;
                config.path = ".";
                ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey", "Choose File", ".glb,.gltf", config);
            }
            if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey", 32, ImVec2(200.0f, 300.0f))) {
                if (ImGuiFileDialog::Instance()->IsOk()) {
                    std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
                    std::string filePath = ImGuiFileDialog::Instance()->GetCurrentPath();

                    _requestedScenePath = filePathName;
                }

                ImGuiFileDialog::Instance()->Close();
            }
            ImGui::Dummy(ImVec2(0.0f, 10.0f));

            ImGui::SeparatorText("Controls:");
            ImGui::BulletText("TAB: switch to menu mode");
            ImGui::Dummy(ImVec2(0.0f, 10.0f));

            ImGui::SeparatorText("Controls for free camera:");
            ImGui::BulletText("WASD: move camera");
            ImGui::BulletText("SHIFT & CTRL: move camera up & down");
            ImGui::BulletText("SCROLL WHEEL: change camera speed");
        }
        ImGui::End();
    }

    // imgui layout for the right hand panel
    {
        ImGui::SetNextWindowPos(ImVec2(_swapchainExtent.width - panelWidth, 0));
        ImGui::SetNextWindowSize(ImVec2(panelWidth, _swapchainExtent.height));

        ImGuiWindowFlags windowFlags =
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoTitleBar;

        ImGui::Begin("Right Hand Panel", nullptr, windowFlags);
        {
            ImGui::Text("Light Settings:");

            ImGui::Dummy(ImVec2(0.0f, 10.0f));
            ImGui::SeparatorText("Light Orbit Center");
            ImGui::BeginGroup();

            float sensitivity = 0.02f;
            static ImVec4 lightOrbitCenter = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
            ImGui::DragFloat("X", &lightOrbitCenter.x, sensitivity);
            ImGui::DragFloat("Y", &lightOrbitCenter.y, sensitivity);
            ImGui::DragFloat("Z", &lightOrbitCenter.z, sensitivity);

            ImGui::EndGroup();

            ImGui::Dummy(ImVec2(0.0f, 10.0f));
            ImGui::SeparatorText("Light #1");
            ImGui::BeginGroup();

                static ImVec4 color0 = ImVec4(103.0f / 255.0f, 18.0f / 255.0f, 211.0f / 255.0f, 1.0f);
                ImGui::ColorEdit3("Color 1", (float*)&color0);

                static float strength0 = 3.5f;
                ImGui::SliderFloat("Strength 1", &strength0, 0.0f, 10.0f);

                static float distance0 = 1.0f;
                ImGui::SliderFloat("Distance 1", &distance0, 0.1f, 20.0f);
                static float azimuthal0 = 0.514f;
                ImGui::SliderFloat("Azimuthal Angle 1", &azimuthal0, -2 * 3.141592f, 2 * 3.141592f);
                static float polar0 = 1.5f;
                ImGui::SliderFloat("Polar Angle 1", &polar0, 0.0f, 3.141592f);

            ImGui::EndGroup();

            ImGui::Dummy(ImVec2(0.0f, 10.0f));
            ImGui::SeparatorText("Light #2");
            ImGui::BeginGroup();

                static ImVec4 color1 = ImVec4(201.0f / 255.0f, 7.0f / 255.0f, 7.0f / 255.0f, 1.0f);
                ImGui::ColorEdit3("Color 2", (float*)&color1);

                static float strength1 = 2.0f;
                ImGui::SliderFloat("Strength 2", &strength1, 0.0f, 10.0f);

                static float distance1 = 1.0f;
                ImGui::SliderFloat("Distance 2", &distance1, 0.1f, 20.0f);
                static float azimuthal1 = -0.22f;
                ImGui::SliderFloat("Azimuthal Angle 2", &azimuthal1, -2 * 3.141592f, 2 * 3.141592f);
                static float polar1 = 1.977f;
                ImGui::SliderFloat("Polar Angle 2", &polar1, 0.0f, 3.141592f);

            ImGui::EndGroup();

            ImGui::Dummy(ImVec2(0.0f, 10.0f));
            ImGui::SeparatorText("Light #3");
            ImGui::BeginGroup();

                static ImVec4 color2 = ImVec4(37.0f / 255.0f, 200.0f / 255.0f, 42.0f / 255.0f, 1.0f);
                ImGui::ColorEdit3("Color 3", (float*)&color2);

                static float strength2 = 1.67f;
                ImGui::SliderFloat("Strength 3", &strength2, 0.0f, 10.0f);

                static float distance2 = 1.0f;
                ImGui::SliderFloat("Distance 3", &distance2, 0.1f, 20.0f);
                static float azimuthal2 = 1.39f;
                ImGui::SliderFloat("Azimuthal Angle 3", &azimuthal2, -2 * 3.141592f, 2 * 3.141592f);
                static float polar2 = 1.5f;
                ImGui::SliderFloat("Polar Angle 3", &polar2, 0.0f, 3.141592f);

            ImGui::EndGroup();

            ImGui::Dummy(ImVec2(0.0f, 10.0f));
            ImGui::SeparatorText("Ambient Light");
            ImGui::BeginGroup();

                static ImVec4 ambientColor = ImVec4(157.0f / 255.0f, 223.0f / 255.0f, 250.0f / 255.0f, 1.0f);
                ImGui::ColorEdit3("Ambient Color", (float*)&ambientColor);

                static float ambientStrength = 0.01f;
                ImGui::SliderFloat("Ambient Strength", &ambientStrength, 0.0f, 0.1f);

            ImGui::EndGroup();

            _light.lightData.position_0 = glm::vec4(lightOrbitCenter.x + distance0 * sin(polar0) * cos(azimuthal0), lightOrbitCenter.y + distance0 * cos(polar0), lightOrbitCenter.z + distance0 * sin(polar0) * sin(azimuthal0), 0.0f);
            _light.lightData.flux_0 = glm::vec4(color0.x, color0.y, color0.z, strength0);

            _light.lightData.position_1 = glm::vec4(lightOrbitCenter.x + distance1 * sin(polar1) * cos(azimuthal1), lightOrbitCenter.y + distance1 * cos(polar1), lightOrbitCenter.z + distance1 * sin(polar1) * sin(azimuthal1), 0.0f);
            _light.lightData.flux_1 = glm::vec4(color1.x, color1.y, color1.z, strength1);

            _light.lightData.position_2 = glm::vec4(lightOrbitCenter.x + distance2 * sin(polar2) * cos(azimuthal2), lightOrbitCenter.y + distance2 * cos(polar2), lightOrbitCenter.z + distance2 * sin(polar2) * sin(azimuthal2), 0.0f);
            _light.lightData.flux_2 = glm::vec4(color2.x, color2.y, color2.z, strength2);

            _light.lightData.ambient = glm::vec4(ambientColor.x, ambientColor.y, ambientColor.z, ambientStrength);
        }
        ImGui::End();
    }

    ImGuiIO& io = ImGui::GetIO();
    ImGui::Render();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }

    vk::RenderPassBeginInfo imguiRenderPassBeginInfo;
    imguiRenderPassBeginInfo.setRenderPass(_imguiRenderPass);
    imguiRenderPassBeginInfo.setFramebuffer(imguiFramebuffer);
    imguiRenderPassBeginInfo.setRenderArea(vk::Rect2D(vk::Offset2D(0, 0), _swapchainExtent));
    vk::ClearValue imguiClearValues[2] = { vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f), vk::ClearDepthStencilValue(1.0f, 0.0f) };
    imguiRenderPassBeginInfo.setClearValues(imguiClearValues);

    _drawCommandBuffers[activeFrame].beginRenderPass(imguiRenderPassBeginInfo, vk::SubpassContents::eInline);

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), *_drawCommandBuffers[activeFrame]);

    _drawCommandBuffers[activeFrame].endRenderPass();

    _drawCommandBuffers[activeFrame].end();
}

void VulkanRenderer::preprocessScene(std::shared_ptr<Scene> scene)
{
    std::unordered_map<AttributeType, int> attributeSizeMap;
    int indexSize = 0;
    int modelMatrixCount = 0;

    for (const auto& mesh : scene->meshes)
    {
        for (const auto& primitive : mesh->primitives)
        {
            indexSize += primitive->indices.size() * sizeof(decltype(primitive->indices)::value_type);
            modelMatrixCount++;

            for (const auto& attribute : primitive->attributes)
            {
                if (attributeSizeMap.find(attribute.attributeType) == attributeSizeMap.end())
                {
                    attributeSizeMap[attribute.attributeType] = attribute.size;
                }
                else
                {
                    attributeSizeMap[attribute.attributeType] += attribute.size;
                }
            }
        }
    }

    if (indexSize <= 0)
    {
        throw std::runtime_error("VulkanRenderer: loaded scene must contain indices");
    }

    _scene.indexGPUBuffer = VulkanGPUBuffer(&_device, _vmaAllocator.getAllocator(), indexSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer);
    _scene.indexStagingBuffer = VulkanStagingBuffer(&_device, _scene.indexGPUBuffer, &_immediateSubmit);

    _scene.modelMatrixGPUBuffer = VulkanGPUBuffer(&_device, _vmaAllocator.getAllocator(), modelMatrixCount * sizeof(glm::mat4), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress);
    _scene.modelMatrixStagingBuffer = VulkanStagingBuffer(&_device, _scene.modelMatrixGPUBuffer, &_immediateSubmit);

    _scene.normalMatrixGPUBuffer = VulkanGPUBuffer(&_device, _vmaAllocator.getAllocator(), modelMatrixCount * sizeof(glm::mat4), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress);
    _scene.normalMatrixStagingBuffer = VulkanStagingBuffer(&_device, _scene.normalMatrixGPUBuffer, &_immediateSubmit);

    for (const auto& attributeSize : attributeSizeMap)
    {
        VertexAttribute vertexAttribute;
        vertexAttribute.type = attributeSize.first;
        vertexAttribute.gpuBuffer = VulkanGPUBuffer(&_device, _vmaAllocator.getAllocator(), attributeSize.second, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress);
        vertexAttribute.gpuBufferAddressCounter = vertexAttribute.gpuBuffer.getAddress(_device);
        vertexAttribute.stagingBuffer = VulkanStagingBuffer(&_device, vertexAttribute.gpuBuffer, &_immediateSubmit);

        _scene.attributes.push_back(std::move(vertexAttribute));
    }
}

void VulkanRenderer::generateDrawablesFromScene(std::shared_ptr<Node> node, glm::mat4 baseTransform)
{
    if (node->mesh != nullptr)
    {
        for (const auto& primitive : node->mesh->primitives)
        {
            VulkanDrawable drawable;

            if (_scene.primitiveDrawableMap.find(primitive) != _scene.primitiveDrawableMap.end())
            {
                drawable = _scene.primitiveDrawableMap[primitive];
            }
            else
            {
                drawable.firstIndex = _scene.indexCounter;
                drawable.indexCount = primitive->indices.size();
                _scene.indexCounter += primitive->indices.size();

                _scene.indexStagingBuffer.pushData(primitive->indices.data(), primitive->indices.size() * sizeof(decltype(primitive->indices)::value_type));

                _scene.modelMatrixStagingBuffer.pushData(&baseTransform, sizeof(glm::mat4));
                drawable.modelMatrixAddress = _scene.modelMatrixGPUBuffer.getAddress(_device) + _scene.drawableCounter * sizeof(glm::mat4);

                // create and push normal matrix
                glm::mat4 normalMatrix = glm::transpose(glm::inverse(baseTransform)); // mat4 for alignment, 4th dimension is dropped in shader
                _scene.normalMatrixStagingBuffer.pushData(&normalMatrix, sizeof(glm::mat4));
                drawable.normalMatrixAddress = _scene.normalMatrixGPUBuffer.getAddress(_device) + _scene.drawableCounter * sizeof(glm::mat4);

                _scene.drawableCounter++;

                for (const auto& attribute : primitive->attributes)
                {
                    auto vertexAttributeIterator = std::find_if(_scene.attributes.begin(), _scene.attributes.end(), [&](const VertexAttribute& vertexAttribute) { return vertexAttribute.type == attribute.attributeType; });
                    vertexAttributeIterator->stagingBuffer.pushData(attribute.elements.get(), attribute.size);
                    drawable.setAddress(attribute.attributeType, vertexAttributeIterator->gpuBufferAddressCounter);
                    vertexAttributeIterator->gpuBufferAddressCounter += attribute.size;
                }

                _scene.primitiveDrawableMap[primitive] = drawable;

                // all scenes loaded using the glTF loader contain the glTFPBR material 
                if (!_scene.contexts.contains(primitive->material->materialTypeName))
                {
                    _scene.contexts[primitive->material->materialTypeName] = VulkanMaterialContext();
                    if (primitive->material->materialTypeName == "glTFPBR")
                    {
                        _scene.glTFPBRMaterial = GLTFPBRMaterial(
                            &_device, _vmaAllocator.getAllocator(), &_immediateSubmit, _renderPass, _globalDescriptorSetLayout,
                            _lightDescriptorSetLayout, &_descriptorAllocator, &_descriptorWriter, _shaderCompiler
                        );
                        _scene.contexts[primitive->material->materialTypeName].pipeline = _scene.glTFPBRMaterial.getPipeline();
                        _scene.contexts[primitive->material->materialTypeName].pipelineLayout = _scene.glTFPBRMaterial.getPipelineLayout();

                        drawable.descriptorSet = _scene.glTFPBRMaterial.createDescriptorSet(primitive->material);
                    }
                    else
                    {
                        throw std::runtime_error("unsupported material type.");
                    }
                }
                else if (primitive->material->materialTypeName == "glTFPBR")
                {
                    drawable.descriptorSet = _scene.glTFPBRMaterial.createDescriptorSet(primitive->material);
                }
                else
                {
                    throw std::runtime_error("unsupported material type.");
                }
            }

            _scene.contexts[primitive->material->materialTypeName].drawables.push_back(drawable);
        }
    }

    for (const auto& child : node->children)
    {
        generateDrawablesFromScene(child, child->transform * baseTransform);
    }
}

void VulkanRenderer::copyStagingBuffersToGPUBuffers()
{
    _scene.indexStagingBuffer.copyToBuffer(_scene.indexGPUBuffer);
    _graphicsQueue.waitIdle();

    _scene.modelMatrixStagingBuffer.copyToBuffer(_scene.modelMatrixGPUBuffer);
    _graphicsQueue.waitIdle();

    _scene.normalMatrixStagingBuffer.copyToBuffer(_scene.normalMatrixGPUBuffer);
    _graphicsQueue.waitIdle();

    for (auto& attribute : _scene.attributes)
    {
        attribute.stagingBuffer.copyToBuffer(attribute.gpuBuffer);
        _graphicsQueue.waitIdle();
    }
}

void VulkanRenderer::recreateSwapchain()
{
    (*_device).waitIdle();

    _framebuffers.clear();
    _imguiFramebuffers.clear();
    _imageViews.clear();
    _swapchain.clear();

    _depthBuffer = VulkanImage();

    _swapchain = _initilization.createSwapchain(_device, _surface);
    _swapchainExtent = _initilization.getSwapchainExtent();
    _swapchainFormat = _initilization.getSwapchainFormat();

    _imageViews = _initilization.createSwapchainImageViews(_device);

    createDepthBuffer();

    _framebuffers = _initilization.createFramebuffers(_device, _renderPass, _imageViews, _depthBuffer.getImageView());
    _imguiFramebuffers = _initilization.createFramebuffers(_device, _imguiRenderPass, _imageViews);

    vk::CommandBufferAllocateInfo commandBufferAllocateInfo;
    commandBufferAllocateInfo.setLevel(vk::CommandBufferLevel::ePrimary);
    commandBufferAllocateInfo.setCommandPool(_commandPool);
    commandBufferAllocateInfo.setCommandBufferCount(_framesInFlight);

    _drawCommandBuffers = vk::raii::CommandBuffers(_device, commandBufferAllocateInfo);
}

void VulkanRenderer::createRenderPass()
{
    vk::AttachmentDescription colorAttachmentDescription;
    colorAttachmentDescription.setFormat(_swapchainFormat);
    colorAttachmentDescription.setSamples(vk::SampleCountFlagBits::e1);
    colorAttachmentDescription.setLoadOp(vk::AttachmentLoadOp::eClear);
    colorAttachmentDescription.setStoreOp(vk::AttachmentStoreOp::eStore);
    colorAttachmentDescription.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare);
    colorAttachmentDescription.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);
    colorAttachmentDescription.setInitialLayout(vk::ImageLayout::eUndefined);
    colorAttachmentDescription.setFinalLayout(vk::ImageLayout::eColorAttachmentOptimal); // for the imgui render pass

    vk::AttachmentDescription depthAttachmentDescription;
    depthAttachmentDescription.setFormat(vk::Format::eD32Sfloat);
    depthAttachmentDescription.setSamples(vk::SampleCountFlagBits::e1);
    depthAttachmentDescription.setLoadOp(vk::AttachmentLoadOp::eClear);
    depthAttachmentDescription.setStoreOp(vk::AttachmentStoreOp::eDontCare);
    depthAttachmentDescription.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare);
    depthAttachmentDescription.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);
    depthAttachmentDescription.setInitialLayout(vk::ImageLayout::eUndefined);
    depthAttachmentDescription.setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

    vk::AttachmentReference colorAttachmentReference;
    colorAttachmentReference.setAttachment(0);
    colorAttachmentReference.setLayout(vk::ImageLayout::eColorAttachmentOptimal);

    vk::AttachmentReference depthAttachmentReference;
    depthAttachmentReference.setAttachment(1);
    depthAttachmentReference.setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

    vk::SubpassDescription subpassDescription;
    subpassDescription.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics);
    subpassDescription.setColorAttachments(colorAttachmentReference);
    subpassDescription.setPDepthStencilAttachment(&depthAttachmentReference);

    vk::AttachmentDescription attachments[2] = { colorAttachmentDescription, depthAttachmentDescription };

    vk::RenderPassCreateInfo renderPassCreateInfo;
    renderPassCreateInfo.setAttachments(attachments);
    renderPassCreateInfo.setSubpasses(subpassDescription);

    _renderPass = vk::raii::RenderPass(_device, renderPassCreateInfo);
}

void VulkanRenderer::createGlobalDescriptorSets()
{
    vk::DescriptorSetLayoutBinding descriptorSetLayoutBinding;

    // camera parameters
    descriptorSetLayoutBinding.setBinding(0);
    descriptorSetLayoutBinding.setDescriptorCount(1);
    descriptorSetLayoutBinding.setDescriptorType(vk::DescriptorType::eUniformBuffer);
    descriptorSetLayoutBinding.setStageFlags(vk::ShaderStageFlagBits::eVertex);

    vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo;
    descriptorSetLayoutCreateInfo.setBindings(descriptorSetLayoutBinding);

    _globalDescriptorSetLayout = vk::raii::DescriptorSetLayout(_device, descriptorSetLayoutCreateInfo);

    for (int i = 0; i < _framesInFlight; i++)
    {
        _globalDescriptorSetBuffers.push_back(VulkanUniformBuffer(&_device, _vmaAllocator.getAllocator(), sizeof(ShaderStructures::GlobalUniformBuffer)));
        _globalDescriptorSets.push_back(std::move(_descriptorAllocator.allocateSet(_globalDescriptorSetLayout)));
    }

    // light parameters
    descriptorSetLayoutBinding.setBinding(0);
    descriptorSetLayoutBinding.setDescriptorCount(1);
    descriptorSetLayoutBinding.setDescriptorType(vk::DescriptorType::eUniformBuffer);
    descriptorSetLayoutBinding.setStageFlags(vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment);

    descriptorSetLayoutCreateInfo.setBindings(descriptorSetLayoutBinding);

    _lightDescriptorSetLayout = vk::raii::DescriptorSetLayout(_device, descriptorSetLayoutCreateInfo);
}

void VulkanRenderer::createDepthBuffer()
{
    _depthBuffer = VulkanImage(&_device, &_immediateSubmit, _vmaAllocator.getAllocator(), _swapchainExtent, vk::Format::eD32Sfloat, vk::ImageAspectFlagBits::eDepth, vk::ImageUsageFlagBits::eDepthStencilAttachment);
}
