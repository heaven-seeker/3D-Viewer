#pragma once

#include <MeshResource.hpp>
#include <Scene.hpp>
#include <optional>

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

    bool initBuffers();
    void initRenderPipeline(wgpu::ShaderModule shader);
    void initSampler();
    void initBindGroups();

    bool setSurface(WGPUSurface nativeSurface);
    bool setupSurfaceConfig(int width, int height);
    void resizeSurface(int width, int height);

    void updateMeshBuffers(const Mesh& model);

    wgpu::TextureView acquireSurfaceTextureView();

    void draw(UILayer& ui, const Camera& camera, const Scene& scene);

    void drawJustUI(UILayer& ui);

    bool updateMaterial(uint32_t meshId, uint32_t materialId,
                        const Material& material);

    std::optional<uint32_t> addMesh(Mesh mesh);
    Mesh* getMesh(uint32_t meshId);

   private:
    MeshResource m_meshResource;
    std::vector<MeshResource> m_meshResources;

    bool updateMaterialBuffer(const Mesh& model, MeshResource& resource);
    uint32_t getMaterialOffset(const MeshResource& resource,
                               int materialId) const;
    bool uploadMeshResource(const Mesh& model, MeshResource& resource);

    wgpu::Instance m_instance;
    wgpu::Device m_device;
    wgpu::Adapter m_adapter;
    wgpu::Queue m_queue;
    wgpu::Surface m_surface;
    wgpu::TextureFormat m_surfaceFormat;

    bool m_hasFailed = false;

    wgpu::Buffer m_uniformBuffer;
    wgpu::Buffer m_lightBuffer;
    std::vector<wgpu::BindGroupLayout> m_bindGroupLayouts;
    wgpu::PipelineLayout m_pipelineLayout;
    std::vector<wgpu::BindGroup> m_bindGroups;

    wgpu::RenderPipeline m_renderPipeline;

    wgpu::Sampler m_sampler;

    wgpu::Texture m_depthTexture;
    wgpu::Texture m_msaaTexture;
    wgpu::TextureView m_msaaTextureView;
    wgpu::TextureView m_depthTextureView;
    wgpu::TextureDescriptor m_depthTextureDescriptor;
    wgpu::TextureViewDescriptor m_depthTextureViewDescriptor;
    wgpu::TextureFormat m_depthTextureFormat;

    uint64_t m_uniformStride = 0;

    std::size_t m_objectCapacity = 64;

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
