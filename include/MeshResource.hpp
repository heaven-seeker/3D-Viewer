#pragma once

#include <webgpu/webgpu_cpp.h>

#include <Mesh.hpp>
#include <cstddef>
#include <cstdint>

namespace C2Lux {

struct MeshResource {
    Mesh model;
    wgpu::Buffer vertexBuffer;
    std::size_t vertexCapacity = 0;

    wgpu::Buffer indexBuffer;
    std::size_t indexCapacity = 0;
    uint32_t indexCount = 0;

    wgpu::Buffer materialBuffer;
    wgpu::BindGroup materialBindGroup;
    uint32_t materialStride = 0;
    uint32_t materialCount = 0;
};

}  // namespace C2Lux