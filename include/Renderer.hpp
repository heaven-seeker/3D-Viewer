#pragma once

#include "Camera.hpp"
#include "Mesh.hpp"
#include "glfw3webgpu.h"
#include "webgpu/webgpu_cpp.h"

namespace C2Lux {

class UILayer;

class Renderer {
   public:
    Renderer();
    // Persistent device callbacks refer to this renderer's address.
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    Renderer(Renderer&&) = delete;
    Renderer& operator=(Renderer&&) = delete;

    bool initInstance();
    bool initAdapter();
    bool initDevice();
    bool initQueue();

    void initBuffers();
    void initRenderPipeline(wgpu::ShaderModule shader);
    void initSampler();
    void initBindGroups();

    bool setSurface(WGPUSurface nativeSurface);
    bool setupSurfaceConfig(int width, int height);
    void resizeSurface(int width, int height);

    void updateMeshBuffers(const Mesh& model);

    wgpu::TextureView acquireSurfaceTextureView();

    void draw(UILayer& ui, const Camera& camera, const Mesh& mesh);

    void drawJustUI(UILayer& ui);

    bool updateMaterial(uint32_t materialId, const Material& material);

   private:
    bool updateMaterialBuffer(const Mesh& model);
    uint32_t getMaterialOffset(int materialId) const;

    wgpu::Instance m_instance;
    wgpu::Device m_device;
    wgpu::Adapter m_adapter;
    wgpu::Queue m_queue;
    wgpu::Surface m_surface;
    wgpu::TextureFormat m_surfaceFormat;

    bool m_hasFailed = false;

    wgpu::Buffer m_vertexBuffer;
    size_t m_vertexBufferCapacity = 1024;

    wgpu::Buffer m_indexBuffer;
    size_t m_indexBufferCapacity = 1024;
    uint32_t m_indexCount = 0;

    wgpu::Buffer m_uniformBuffer;
    wgpu::Buffer m_lightBuffer;
    std::vector<wgpu::BindGroupLayout> m_bindGroupLayouts;
    wgpu::PipelineLayout m_pipelineLayout;
    std::vector<wgpu::BindGroup> m_bindGroups;

    // Slot 0 is white; material i occupies slot i + 1 in this shared buffer.
    wgpu::Buffer m_materialBuffer;
    wgpu::BindGroup m_materialBindGroup;
    uint32_t m_materialStride = 0;
    size_t m_materialCount = 0;

    wgpu::RenderPipeline m_renderPipeline;

    wgpu::Sampler m_sampler;

    wgpu::Texture m_depthTexture;
    wgpu::Texture m_msaaTexture;
    wgpu::TextureView m_msaaTextureView;
    wgpu::TextureView m_depthTextureView;
    wgpu::TextureDescriptor m_depthTextureDescriptor;
    wgpu::TextureViewDescriptor m_depthTextureViewDescriptor;
    wgpu::TextureFormat m_depthTextureFormat;

   public:
    const wgpu::Instance& getInstance() const { return m_instance; }
    const wgpu::Device& getDevice() const { return m_device; }
    wgpu::TextureFormat getSurfaceFormat() const { return m_surfaceFormat; }

    // Process asynchronous callbacks even when the window is minimized.
    void processEvents() {
        if (m_instance) m_instance.ProcessEvents();
    }
    bool hasFailed() const { return m_hasFailed; }

    wgpu::SurfaceConfiguration m_surfaceConfig{};
};

}  // namespace C2Lux
