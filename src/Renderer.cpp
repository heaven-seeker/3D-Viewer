#include "Renderer.hpp"

#include "CoreData.hpp"
#include "UILayer.hpp"
#include "c2m/c2m.hpp"
#include "glm/ext.hpp"

// std
#include <chrono>
#include <cmath>
#include <cstring>
#include <iostream>
#include <limits>
#include <string_view>
#include <thread>

namespace C2Lux {

Renderer::Renderer() = default;

bool Renderer::initInstance() {
    wgpu::InstanceDescriptor instanceDesc{};
    m_instance = wgpu::CreateInstance(&instanceDesc);
    if (!m_instance) {
        std::cerr << "Could not intialize WebGPU!" << std::endl;
        return false;
    }
    return true;
}

bool Renderer::setSurface(WGPUSurface nativeSurface) {
    if (!nativeSurface) {
        std::cerr << "Could not set surface!" << std::endl;
        return false;
    }
    m_surface = wgpu::Surface::Acquire(nativeSurface);
    return true;
}

bool Renderer::initAdapter() {
    wgpu::RequestAdapterOptions adapterOpts{};
    adapterOpts.nextInChain = nullptr;
    adapterOpts.compatibleSurface = m_surface;

    bool requestEnded = false;

    m_instance.RequestAdapter(
        &adapterOpts, wgpu::CallbackMode::AllowProcessEvents,
        [this, &requestEnded](wgpu::RequestAdapterStatus status,
                              wgpu::Adapter requestedAdapter,
                              wgpu::StringView message) {
            if (status == wgpu::RequestAdapterStatus::Success) {
                m_adapter = requestedAdapter;
                wgpu::AdapterInfo info{};
                if (m_adapter.GetInfo(&info) == wgpu::Status::Success) {
                    std::cout
                        << "WebGPU adapter: " << std::string_view(info.device)
                        << std::endl;
                }
            } else {
                std::cerr << "Could not get WebGPU adapter: ";
                std::cerr << std::string_view(message);
                std::cerr << std::endl;
            }
            requestEnded = true;
        });

    while (!requestEnded) {
        m_instance.ProcessEvents();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    return m_adapter != nullptr;
}

bool Renderer::initDevice() {
    wgpu::DeviceDescriptor deviceDesc{};
    deviceDesc.nextInChain = nullptr;
    deviceDesc.label = "My device";
    deviceDesc.requiredFeatureCount = 0;
    deviceDesc.requiredLimits = nullptr;
    deviceDesc.defaultQueue.nextInChain = nullptr;
    deviceDesc.defaultQueue.label = "The default queue";

    // Register callbacks on the descriptor before requesting the device.
    deviceDesc.SetDeviceLostCallback(
        wgpu::CallbackMode::AllowProcessEvents,
        [this](const wgpu::Device&, wgpu::DeviceLostReason reason,
               wgpu::StringView message) {
            if (reason == wgpu::DeviceLostReason::Destroyed ||
                reason == wgpu::DeviceLostReason::CallbackCancelled)
                return;
            m_hasFailed = true;
            std::cerr << "Device lost (" << static_cast<uint32_t>(reason)
                      << "): " << std::string_view(message) << std::endl;
        });
    deviceDesc.SetUncapturedErrorCallback([](const wgpu::Device&,
                                             wgpu::ErrorType type,
                                             wgpu::StringView message) {
        std::cerr << "WebGPU error (" << static_cast<uint32_t>(type)
                  << "): " << std::string_view(message) << std::endl;
    });
    // WebGPU's default limits cover the current viewer. Do not cap meshes at
    // 150 KB or render targets at 2048 pixels as the old manual limits did.

    bool requestEnded = false;

    m_adapter.RequestDevice(
        &deviceDesc, wgpu::CallbackMode::AllowProcessEvents,
        [this, &requestEnded](wgpu::RequestDeviceStatus status,
                              wgpu::Device requestedDevice,
                              wgpu::StringView message) {
            if (status == wgpu::RequestDeviceStatus::Success) {
                m_device = requestedDevice;
            } else {
                std::cerr << "Could not get WebGPU device: ";
                std::cerr << std::string_view(message);
                std::cerr << std::endl;
            }
            requestEnded = true;
        });

    while (!requestEnded) {
        m_instance.ProcessEvents();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    return m_device != nullptr;
}

bool Renderer::initQueue() {
    m_queue = m_device.GetQueue();

    return m_queue != nullptr;
}

void Renderer::initBindGroups() {
    std::vector<wgpu::BindGroupLayoutEntry> bindingLayoutEntries(1);

    // Uniforms
    wgpu::BindGroupLayoutEntry& uniformBindingLayout = bindingLayoutEntries[0];
    uniformBindingLayout.binding = 0;
    uniformBindingLayout.visibility =
        wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
    uniformBindingLayout.buffer.type = wgpu::BufferBindingType::Uniform;
    uniformBindingLayout.buffer.minBindingSize = sizeof(Uniforms);
    uniformBindingLayout.buffer.hasDynamicOffset =
        false;  // NOTE: Need change in future

    // Light
    wgpu::BindGroupLayoutEntry lightBindingLayout = {};
    lightBindingLayout.binding = 0;
    lightBindingLayout.visibility = wgpu::ShaderStage::Fragment;
    lightBindingLayout.buffer.type = wgpu::BufferBindingType::Uniform;
    lightBindingLayout.buffer.minBindingSize = sizeof(LightData);
    lightBindingLayout.buffer.hasDynamicOffset = false;

    // Uniforms
    wgpu::BindGroupLayoutDescriptor bindGroupLayoutDescriptor = {};
    bindGroupLayoutDescriptor.nextInChain = nullptr;
    bindGroupLayoutDescriptor.entryCount =
        static_cast<uint32_t>(bindingLayoutEntries.size());
    bindGroupLayoutDescriptor.entries = bindingLayoutEntries.data();
    m_bindGroupLayouts.push_back(
        m_device.CreateBindGroupLayout(&bindGroupLayoutDescriptor));

    // Light
    wgpu::BindGroupLayoutDescriptor lightbindGroupLayoutDescriptor = {};
    lightbindGroupLayoutDescriptor.nextInChain = nullptr;
    lightbindGroupLayoutDescriptor.entryCount = 1;
    lightbindGroupLayoutDescriptor.entries = &lightBindingLayout;
    m_bindGroupLayouts.push_back(
        m_device.CreateBindGroupLayout(&lightbindGroupLayoutDescriptor));

    // Group 2 selects a material for each submesh draw.
    wgpu::BindGroupLayoutEntry materialBinding{};
    materialBinding.binding = 0;
    materialBinding.visibility = wgpu::ShaderStage::Fragment;
    materialBinding.buffer.type = wgpu::BufferBindingType::Uniform;
    materialBinding.buffer.minBindingSize = sizeof(MaterialUniforms);
    materialBinding.buffer.hasDynamicOffset = true;

    wgpu::BindGroupLayoutDescriptor materialLayoutDescriptor{};
    materialLayoutDescriptor.entryCount = 1;
    materialLayoutDescriptor.entries = &materialBinding;
    m_bindGroupLayouts.push_back(
        m_device.CreateBindGroupLayout(&materialLayoutDescriptor));

    wgpu::PipelineLayoutDescriptor layoutDescriptor{};
    layoutDescriptor.nextInChain = nullptr;
    layoutDescriptor.bindGroupLayoutCount =
        static_cast<uint32_t>(m_bindGroupLayouts.size());
    layoutDescriptor.bindGroupLayouts = m_bindGroupLayouts.data();
    m_pipelineLayout = m_device.CreatePipelineLayout(&layoutDescriptor);

    std::vector<wgpu::BindGroupEntry> bindings(2);

    bindings[0].binding = 0;  // The index of the binding (the entries in
                              // bindGroupDesc can be in any order)
    bindings[0].buffer = m_uniformBuffer;  // The buffer it is actually bound to
    bindings[0].offset =
        0;  // We can specify an offset within the buffer, so that a single
            // buffer can hold, multiple uniform blocks.
    bindings[0].size =
        sizeof(Uniforms);  // And we specify again the size of the buffer.

    bindings[1].binding = 0;
    bindings[1].buffer = m_lightBuffer;
    bindings[1].offset = 0;
    bindings[1].size = sizeof(LightData);

    wgpu::BindGroupDescriptor bindGroupDescriptor{};
    bindGroupDescriptor.nextInChain = nullptr;
    bindGroupDescriptor.layout = m_bindGroupLayouts[0];
    bindGroupDescriptor.entryCount = (uint32_t)bindings.size() - 1;
    bindGroupDescriptor.entries = &bindings[0];
    m_bindGroups.push_back(m_device.CreateBindGroup(&bindGroupDescriptor));

    bindGroupDescriptor.layout = m_bindGroupLayouts[1];
    bindGroupDescriptor.entryCount = 1;
    bindGroupDescriptor.entries = &bindings[1];
    m_bindGroups.push_back(m_device.CreateBindGroup(&bindGroupDescriptor));
}

bool Renderer::setupSurfaceConfig(int width, int height) {
    if (width <= 0 || height <= 0) return false;
    m_surfaceConfig.nextInChain = nullptr;
    m_surfaceConfig.device = m_device;
    m_surfaceConfig.usage = wgpu::TextureUsage::RenderAttachment;
    m_surfaceConfig.presentMode = wgpu::PresentMode::Fifo;
    m_surfaceConfig.viewFormatCount = 0;
    m_surfaceConfig.viewFormats = nullptr;

    wgpu::SurfaceCapabilities capabilities{};
    if (m_surface.GetCapabilities(m_adapter, &capabilities) !=
            wgpu::Status::Success ||
        capabilities.formatCount == 0 || capabilities.alphaModeCount == 0) {
        std::cerr << "Could not query surface capabilities." << std::endl;
        return false;
    }
    m_surfaceConfig.format = capabilities.formats[0];
    m_surfaceConfig.alphaMode = capabilities.alphaModes[0];
    m_surfaceFormat = m_surfaceConfig.format;
    m_surfaceConfig.width = static_cast<uint32_t>(width);
    m_surfaceConfig.height = static_cast<uint32_t>(height);
    m_surface.Configure(&m_surfaceConfig);
    return true;
}

void Renderer::draw(UILayer& ui, const Camera& camera, const Mesh& mesh) {
    Uniforms uniforms{};
    uniforms.time = static_cast<float>(glfwGetTime());
    uniforms.modelMatrix = glm::mat4(1.0f);
    uniforms.viewMatrix = camera.getViewMatrix();
    float aspect = static_cast<float>(m_surfaceConfig.width) /
                   static_cast<float>(m_surfaceConfig.height);
    uniforms.projectionMatrix = camera.getProjectionMatrix(aspect);
    uniforms.cameraPosition = camera.getPosition();
    uniforms.color = {ui.m_modelColor.x, ui.m_modelColor.y, ui.m_modelColor.z,
                      ui.m_modelColor.w};

    LightData lightData{};
    lightData.direction = {ui.m_lightDirection[0], ui.m_lightDirection[1],
                           ui.m_lightDirection[2]};
    lightData.ambient = ui.m_ambientStrength;
    lightData.intensity = ui.m_lightIntensity;
    lightData.shininess = ui.m_shininess;
    lightData.specularStrength = ui.m_specularStrength;

    m_queue.WriteBuffer(m_lightBuffer, 0, &lightData, sizeof(LightData));
    m_queue.WriteBuffer(m_uniformBuffer, 0, &uniforms, sizeof(Uniforms));

    wgpu::TextureView targetView = acquireSurfaceTextureView();
    if (!targetView) {
        ui.endFrame();
        return;
    }

    wgpu::CommandEncoderDescriptor encoderDesc = {};
    encoderDesc.nextInChain = nullptr;
    encoderDesc.label = "Command encoder";
    wgpu::CommandEncoder encoder = m_device.CreateCommandEncoder(&encoderDesc);

    wgpu::RenderPassColorAttachment colorAttachment = {};
    colorAttachment.view = m_msaaTextureView;
    colorAttachment.resolveTarget = targetView;
    colorAttachment.loadOp = wgpu::LoadOp::Clear;
    colorAttachment.storeOp = wgpu::StoreOp::Discard;
    colorAttachment.clearValue =
        wgpu::Color{ui.m_backgroundColor.x, ui.m_backgroundColor.y,
                    ui.m_backgroundColor.z, ui.m_backgroundColor.w};

    wgpu::RenderPassDepthStencilAttachment depthAttachment{};
    depthAttachment.view = m_depthTextureView;
    depthAttachment.depthClearValue = 1.0f;
    depthAttachment.depthLoadOp = wgpu::LoadOp::Clear;
    depthAttachment.depthStoreOp = wgpu::StoreOp::Discard;
    depthAttachment.depthReadOnly = false;
    depthAttachment.stencilClearValue = 0;
    depthAttachment.stencilLoadOp = wgpu::LoadOp::Undefined;
    depthAttachment.stencilStoreOp = wgpu::StoreOp::Undefined;
    depthAttachment.stencilReadOnly = true;

    wgpu::RenderPassDescriptor pass1Desc = {};
    pass1Desc.colorAttachmentCount = 1;
    pass1Desc.colorAttachments = &colorAttachment;
    pass1Desc.depthStencilAttachment = &depthAttachment;

    wgpu::RenderPassEncoder pass1 = encoder.BeginRenderPass(&pass1Desc);
    pass1.SetPipeline(m_renderPipeline);
    if (m_indexCount > 0 && m_vertexBuffer && m_indexBuffer) {
        pass1.SetVertexBuffer(0, m_vertexBuffer);
        pass1.SetIndexBuffer(
            m_indexBuffer, wgpu::IndexFormat::Uint32, 0,
            static_cast<uint64_t>(m_indexCount) * sizeof(uint32_t));
        pass1.SetBindGroup(0, m_bindGroups[0]);
        pass1.SetBindGroup(1, m_bindGroups[1]);

        for (const auto& submesh : mesh.getSubMeshes()) {
            // Same buffer and bind group, but a different material window.
            const uint32_t offset = getMaterialOffset(submesh.materialId);
            pass1.SetBindGroup(2, m_materialBindGroup, 1, &offset);
            pass1.DrawIndexed(submesh.indexCount, 1, submesh.startIndex, 0, 0);
        }
    }
    pass1.End();

    /* Render imGui UI */
    wgpu::RenderPassColorAttachment uiAttachment = {};
    uiAttachment.view = targetView;
    uiAttachment.resolveTarget = nullptr;
    uiAttachment.loadOp = wgpu::LoadOp::Load;
    uiAttachment.storeOp = wgpu::StoreOp::Store;

    wgpu::RenderPassDescriptor pass2Desc = {};
    pass2Desc.colorAttachmentCount = 1;
    pass2Desc.colorAttachments = &uiAttachment;
    pass2Desc.depthStencilAttachment = nullptr;

    wgpu::RenderPassEncoder pass2 = encoder.BeginRenderPass(&pass2Desc);
    ui.draw(pass2);
    pass2.End();

    wgpu::CommandBufferDescriptor cmdBufferDesc = {};
    cmdBufferDesc.nextInChain = nullptr;
    cmdBufferDesc.label = "Command buffer";
    wgpu::CommandBuffer command = encoder.Finish(&cmdBufferDesc);

    // std::cout << "Submitting command..." << std::endl;
    m_queue.Submit(1, &command);

    targetView = nullptr;

    m_surface.Present();

    // std::cout << "Command submitted." << std::endl;
}

void Renderer::drawJustUI(UILayer& ui) {
    wgpu::TextureView targetView = acquireSurfaceTextureView();
    if (!targetView) {
        ui.endFrame();
        return;
    }

    wgpu::CommandEncoderDescriptor encoderDesc = {};
    encoderDesc.nextInChain = nullptr;
    encoderDesc.label = "Command encoder";
    wgpu::CommandEncoder encoder = m_device.CreateCommandEncoder(&encoderDesc);

    wgpu::RenderPassColorAttachment uiAttachment = {};
    uiAttachment.view = targetView;
    uiAttachment.resolveTarget = nullptr;
    uiAttachment.loadOp = wgpu::LoadOp::Clear;
    uiAttachment.storeOp = wgpu::StoreOp::Store;

    wgpu::RenderPassDescriptor uiPassDesc = {};
    uiPassDesc.colorAttachmentCount = 1;
    uiPassDesc.colorAttachments = &uiAttachment;
    uiPassDesc.depthStencilAttachment = nullptr;

    wgpu::RenderPassEncoder uiPass = encoder.BeginRenderPass(&uiPassDesc);
    ui.draw(uiPass);
    uiPass.End();

    wgpu::CommandBufferDescriptor cmdBufferDesc = {};
    cmdBufferDesc.nextInChain = nullptr;
    cmdBufferDesc.label = "Command buffer";
    wgpu::CommandBuffer command = encoder.Finish(&cmdBufferDesc);

    // std::cout << "Submitting command..." << std::endl;
    m_queue.Submit(1, &command);

    targetView = nullptr;

    m_surface.Present();

    // std::cout << "Command submitted." << std::endl;
}

wgpu::TextureView Renderer::acquireSurfaceTextureView() {
    wgpu::SurfaceTexture surfaceTexture{};
    m_surface.GetCurrentTexture(&surfaceTexture);

    switch (surfaceTexture.status) {
        case wgpu::SurfaceGetCurrentTextureStatus::SuccessOptimal:
        case wgpu::SurfaceGetCurrentTextureStatus::SuccessSuboptimal:
            break;
        case wgpu::SurfaceGetCurrentTextureStatus::Timeout:
            return nullptr;  // A temporary failure: skip this frame.
        case wgpu::SurfaceGetCurrentTextureStatus::Outdated:
        case wgpu::SurfaceGetCurrentTextureStatus::Lost:
            // Reconfigure before acquiring a texture on the next frame.
            m_surface.Configure(&m_surfaceConfig);
            return nullptr;
        default:
            m_hasFailed = true;
            std::cerr << "Could not acquire surface texture (status "
                      << static_cast<uint32_t>(surfaceTexture.status) << ")."
                      << std::endl;
            return nullptr;
    }
    if (!surfaceTexture.texture) return nullptr;

    wgpu::TextureViewDescriptor viewDescriptor;
    viewDescriptor.nextInChain = nullptr;
    viewDescriptor.label = "Surface texture view";
    viewDescriptor.format = surfaceTexture.texture.GetFormat();
    viewDescriptor.dimension = wgpu::TextureViewDimension::e2D;
    viewDescriptor.baseMipLevel = 0;
    viewDescriptor.mipLevelCount = 1;
    viewDescriptor.baseArrayLayer = 0;
    viewDescriptor.arrayLayerCount = 1;
    viewDescriptor.aspect = wgpu::TextureAspect::All;

    return surfaceTexture.texture.CreateView(&viewDescriptor);
}

void Renderer::resizeSurface(int width, int height) {
    if (width <= 0 || height <= 0) return;

    m_msaaTextureView = nullptr;
    m_depthTextureView = nullptr;
    m_msaaTexture = nullptr;
    m_depthTexture = nullptr;

    m_surfaceConfig.width = static_cast<uint32_t>(width);
    m_surfaceConfig.height = static_cast<uint32_t>(height);
    m_surface.Configure(&m_surfaceConfig);

    wgpu::TextureDescriptor msaaDesc{};
    msaaDesc.size = {m_surfaceConfig.width, m_surfaceConfig.height, 1};
    msaaDesc.sampleCount = 4;
    msaaDesc.format = m_surfaceFormat;
    msaaDesc.usage = wgpu::TextureUsage::RenderAttachment;
    m_msaaTexture = m_device.CreateTexture(&msaaDesc);
    m_msaaTextureView = m_msaaTexture.CreateView();

    m_depthTextureDescriptor.size = {m_surfaceConfig.width,
                                     m_surfaceConfig.height, 1};
    m_depthTextureDescriptor.sampleCount = 4;
    m_depthTexture = m_device.CreateTexture(&m_depthTextureDescriptor);
    m_depthTextureView = m_depthTexture.CreateView();
}

bool Renderer::updateMaterialBuffer(const Mesh& model) {
    wgpu::Limits limits{};
    if (m_device.GetLimits(&limits) != wgpu::Status::Success) {
        std::cerr << "Could not query material buffer alignment." << std::endl;
        return false;
    }

    // Payload size and slot spacing differ: each dynamic offset must satisfy
    // the device's uniform-buffer offset alignment.
    const uint32_t alignment = limits.minUniformBufferOffsetAlignment;
    m_materialStride = static_cast<uint32_t>(
        (sizeof(MaterialUniforms) + alignment - 1) / alignment * alignment);

    const size_t materialCount = model.getMaterials().size();
    const uint64_t slotCount =
        static_cast<uint64_t>(materialCount) + 1;  // + 1 default white material
    if (materialCount >
            std::numeric_limits<uint32_t>::max() / m_materialStride ||
        slotCount > limits.maxBufferSize / m_materialStride) {
        std::cerr
            << "Materials exceed the device's dynamic uniform buffer limits."
            << std::endl;
        return false;
    }
    const uint64_t bufferSize = slotCount * m_materialStride;

    // Pack all colors once on upload. Slot 0 is the white fallback.
    std::vector<uint8_t> data(static_cast<size_t>(bufferSize), 0);
    MaterialUniforms uniforms{};
    std::memcpy(data.data(), &uniforms, sizeof(uniforms));
    for (size_t i = 0; i < materialCount; ++i) {
        const Material& material = model.getMaterials()[i];

        uniforms.baseColor = glm::vec4(material.diffuseColor, 1.0f);
        uniforms.shininess = material.shininess;
        uniforms.specularStrength = material.specularStrength;
        uniforms.specularColor = material.specularColor;

        std::memcpy(data.data() + (i + 1) * m_materialStride, &uniforms,
                    sizeof(uniforms));
    }

    wgpu::BufferDescriptor bufferDescriptor{};
    bufferDescriptor.label = "All material colors";
    bufferDescriptor.size = bufferSize;
    bufferDescriptor.usage =
        wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst;
    m_materialBuffer = m_device.CreateBuffer(&bufferDescriptor);
    m_queue.WriteBuffer(m_materialBuffer, 0, data.data(), data.size());

    wgpu::BindGroupEntry binding{};
    binding.binding = 0;
    binding.buffer = m_materialBuffer;
    binding.offset = 0;
    // This is the shader-visible window size, NOT the whole buffer size.
    binding.size = sizeof(MaterialUniforms);

    wgpu::BindGroupDescriptor bindGroupDescriptor{};
    bindGroupDescriptor.label = "All material colors, dynamic offset";
    bindGroupDescriptor.layout = m_bindGroupLayouts[2];
    bindGroupDescriptor.entryCount = 1;
    bindGroupDescriptor.entries = &binding;
    m_materialBindGroup = m_device.CreateBindGroup(&bindGroupDescriptor);
    m_materialCount = materialCount;
    return true;
}

uint32_t Renderer::getMaterialOffset(int materialId) const {
    if (materialId < 0 || static_cast<size_t>(materialId) >= m_materialCount) {
        return 0;
    }
    return (static_cast<uint32_t>(materialId) + 1) * m_materialStride;
}

bool Renderer::updateMaterial(uint32_t materialId, const Material& material) {
    if (!m_materialBuffer || materialId >= m_materialCount) {
        return false;
    }

    MaterialUniforms uniforms{};
    uniforms.baseColor = glm::vec4(material.diffuseColor, 1.0f);
    uniforms.shininess = material.shininess;
    uniforms.specularStrength = material.specularStrength;
    uniforms.specularColor = material.specularColor;

    const uint64_t offset =
        (static_cast<uint64_t>(materialId) + 1) * m_materialStride;

    m_queue.WriteBuffer(m_materialBuffer, offset, &uniforms, sizeof(uniforms));

    return true;
}

void Renderer::updateMeshBuffers(const Mesh& model) {
    if (!updateMaterialBuffer(model)) {
        m_hasFailed = true;
        m_indexCount = 0;
        return;
    }

    if (!m_vertexBuffer ||
        model.getVertexData().size() >= m_vertexBufferCapacity) {
        if (model.getVertexData().size() >= m_vertexBufferCapacity) {
            m_vertexBufferCapacity =
                model.getVertexData().size() + model.getVertexData().size() / 2;
        }
        if (m_vertexBuffer) {
            m_vertexBuffer.Destroy();
        }

        wgpu::BufferDescriptor bufferDesc{};
        bufferDesc.nextInChain = nullptr;
        bufferDesc.size = m_vertexBufferCapacity * sizeof(VertexAttributes);
        bufferDesc.label = "Vertex buffer";
        bufferDesc.usage =
            wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Vertex;
        bufferDesc.mappedAtCreation = false;
        m_vertexBuffer = m_device.CreateBuffer(&bufferDesc);
    }

    if (!m_indexBuffer ||
        model.getIndexData().size() >= m_indexBufferCapacity) {
        if (model.getIndexData().size() >= m_indexBufferCapacity) {
            m_indexBufferCapacity =
                model.getIndexData().size() + model.getIndexData().size() / 2;
        }
        if (m_indexBuffer) {
            m_indexBuffer.Destroy();
        }

        wgpu::BufferDescriptor indexBufferDesc{};
        indexBufferDesc.nextInChain = nullptr;
        indexBufferDesc.size =
            m_indexBufferCapacity *
            sizeof(uint32_t);  // default m_indexBufferCapacity
        indexBufferDesc.label = "Index buffer";
        indexBufferDesc.usage =
            wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Index;
        indexBufferDesc.mappedAtCreation = false;
        m_indexBuffer = m_device.CreateBuffer(&indexBufferDesc);
    }

    m_queue.WriteBuffer(
        m_vertexBuffer, 0, model.getVertexData().data(),
        model.getVertexData().size() * sizeof(VertexAttributes));

    m_queue.WriteBuffer(m_indexBuffer, 0, model.getIndexData().data(),
                        model.getIndexData().size() * sizeof(uint32_t));
    m_indexCount = static_cast<uint32_t>(model.getIndexData().size());
}

void Renderer::initBuffers() {
    wgpu::BufferDescriptor uniformBufferDesc{};
    uniformBufferDesc.nextInChain = nullptr;
    uniformBufferDesc.size = sizeof(Uniforms);
    uniformBufferDesc.label = "Uniforms buffer";
    uniformBufferDesc.mappedAtCreation = false;
    uniformBufferDesc.usage =
        wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform;
    m_uniformBuffer = m_device.CreateBuffer(&uniformBufferDesc);

    wgpu::BufferDescriptor lightBufferDesc{};
    lightBufferDesc.nextInChain = nullptr;
    lightBufferDesc.mappedAtCreation = false;
    lightBufferDesc.label = "Light buffer";
    lightBufferDesc.size = c2m::ceilToMultipleOf16(sizeof(LightData));
    lightBufferDesc.usage =
        wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform;
    m_lightBuffer = m_device.CreateBuffer(&lightBufferDesc);
}

void Renderer::initRenderPipeline(wgpu::ShaderModule shader) {
    shader.GetCompilationInfo(
        wgpu::CallbackMode::AllowProcessEvents,
        [](wgpu::CompilationInfoRequestStatus status,
           const wgpu::CompilationInfo* compilationInfo) {
            if (status != wgpu::CompilationInfoRequestStatus::Success ||
                !compilationInfo) {
                std::cerr << "Could not retrieve WGSL compilation info."
                          << std::endl;
                return;
            }
            if (compilationInfo->messageCount > 0) {
                std::cout << "--- WGSL Compilation Info ---" << std::endl;
                for (size_t i = 0; i < compilationInfo->messageCount; ++i) {
                    const auto& msg = compilationInfo->messages[i];
                    std::cout << "Line " << msg.lineNum << ":" << msg.linePos
                              << " - " << std::string_view(msg.message)
                              << std::endl;
                }
                std::cout << "-----------------------------" << std::endl;
            }
        });

    wgpu::RenderPipelineDescriptor pipelineDesc{};
    pipelineDesc.nextInChain = nullptr;
    pipelineDesc.layout = m_pipelineLayout;

    wgpu::VertexBufferLayout vertexBufferLayout{};

    std::vector<wgpu::VertexAttribute> vertexAttributes(4);

    vertexAttributes[0].shaderLocation = 0;
    vertexAttributes[0].format = wgpu::VertexFormat::Float32x3;
    vertexAttributes[0].offset = offsetof(VertexAttributes, position);

    vertexAttributes[1].shaderLocation = 1;
    vertexAttributes[1].format = wgpu::VertexFormat::Float32x3;
    vertexAttributes[1].offset = offsetof(VertexAttributes, normal);

    vertexAttributes[2].shaderLocation = 2;
    vertexAttributes[2].format = wgpu::VertexFormat::Float32x3;
    vertexAttributes[2].offset = offsetof(VertexAttributes, color);

    vertexAttributes[3].shaderLocation = 3;
    vertexAttributes[3].format = wgpu::VertexFormat::Float32x2;
    vertexAttributes[3].offset = offsetof(VertexAttributes, uv);

    vertexBufferLayout.arrayStride = sizeof(VertexAttributes);
    vertexBufferLayout.attributeCount =
        static_cast<uint32_t>(vertexAttributes.size());
    vertexBufferLayout.attributes = vertexAttributes.data();
    vertexBufferLayout.stepMode = wgpu::VertexStepMode::Vertex;

    pipelineDesc.vertex.bufferCount = 1;
    pipelineDesc.vertex.buffers = &vertexBufferLayout;
    pipelineDesc.vertex.module = shader;
    pipelineDesc.vertex.entryPoint = "vs_main";
    pipelineDesc.vertex.constantCount = 0;
    pipelineDesc.vertex.constants = nullptr;

    pipelineDesc.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
    pipelineDesc.primitive.stripIndexFormat = wgpu::IndexFormat::Undefined;
    pipelineDesc.primitive.frontFace = wgpu::FrontFace::CCW;
    pipelineDesc.primitive.cullMode = wgpu::CullMode::None;

    wgpu::FragmentState fragmentState{};
    fragmentState.module = shader;
    fragmentState.entryPoint = "fs_main";
    fragmentState.constantCount = 0;
    fragmentState.constants = nullptr;

    wgpu::BlendState blendState{};
    blendState.color.srcFactor = wgpu::BlendFactor::SrcAlpha;
    blendState.color.dstFactor = wgpu::BlendFactor::OneMinusSrcAlpha;
    blendState.color.operation = wgpu::BlendOperation::Add;
    // Similar a = srcFactor * srcA [operation] dstFactor * destA
    blendState.alpha.srcFactor = wgpu::BlendFactor::Zero;
    blendState.alpha.dstFactor = wgpu::BlendFactor::One;
    blendState.alpha.operation = wgpu::BlendOperation::Add;
    wgpu::ColorTargetState colorTarget{};
    colorTarget.format = m_surfaceFormat;
    colorTarget.blend = &blendState;
    colorTarget.writeMask = wgpu::ColorWriteMask::All;

    fragmentState.targetCount = 1;
    fragmentState.targets = &colorTarget;

    pipelineDesc.fragment = &fragmentState;

    wgpu::DepthStencilState depthStencilState{};
    depthStencilState.depthCompare =
        wgpu::CompareFunction::Less;  // compare two z-buffer values
    depthStencilState.depthWriteEnabled =
        true;  // overwrite value = a if a less than b. Can deactivate when
               // rendering user interface for example
    m_depthTextureFormat = wgpu::TextureFormat::Depth24Plus;
    depthStencilState.format = m_depthTextureFormat;
    depthStencilState.stencilReadMask = 0;
    depthStencilState.stencilWriteMask = 0;

    m_depthTextureDescriptor = {};
    m_depthTextureDescriptor.dimension = wgpu::TextureDimension::e2D;
    m_depthTextureDescriptor.format = m_depthTextureFormat;
    m_depthTextureDescriptor.mipLevelCount = 1;
    m_depthTextureDescriptor.sampleCount = 4;
    m_depthTextureDescriptor.size = {m_surfaceConfig.width,
                                     m_surfaceConfig.height, 1};
    m_depthTextureDescriptor.usage = wgpu::TextureUsage::RenderAttachment;
    m_depthTextureDescriptor.viewFormatCount = 1;
    m_depthTextureDescriptor.viewFormats = &m_depthTextureFormat;
    m_depthTexture = m_device.CreateTexture(&m_depthTextureDescriptor);

    wgpu::TextureDescriptor msaaDesc{};
    msaaDesc.size = {m_surfaceConfig.width, m_surfaceConfig.height, 1};
    msaaDesc.sampleCount = 4;
    msaaDesc.format = m_surfaceFormat;
    msaaDesc.usage = wgpu::TextureUsage::RenderAttachment;
    m_msaaTexture = m_device.CreateTexture(&msaaDesc);
    m_msaaTextureView = m_msaaTexture.CreateView();

    m_depthTextureView =
        m_depthTexture.CreateView(&m_depthTextureViewDescriptor);

    pipelineDesc.depthStencil = &depthStencilState;

    pipelineDesc.multisample.count = 4;
    pipelineDesc.multisample.mask = ~0u;
    pipelineDesc.multisample.alphaToCoverageEnabled = false;

    m_renderPipeline = m_device.CreateRenderPipeline(&pipelineDesc);
}

void Renderer::initSampler() {
    wgpu::SamplerDescriptor samplerDescriptor{};
    samplerDescriptor.addressModeU = wgpu::AddressMode::Repeat;
    samplerDescriptor.addressModeV = wgpu::AddressMode::Repeat;
    samplerDescriptor.addressModeW = wgpu::AddressMode::ClampToEdge;
    samplerDescriptor.magFilter = wgpu::FilterMode::Linear;
    samplerDescriptor.minFilter = wgpu::FilterMode::Linear;
    samplerDescriptor.mipmapFilter = wgpu::MipmapFilterMode::Linear;
    samplerDescriptor.lodMinClamp = 0.0f;
    samplerDescriptor.lodMaxClamp = 8.0f;
    samplerDescriptor.compare = wgpu::CompareFunction::Undefined;
    samplerDescriptor.maxAnisotropy = 16;
    m_sampler = m_device.CreateSampler(&samplerDescriptor);
}

}  // namespace C2Lux
