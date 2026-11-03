#pragma once

#include "Mesh.hpp"
#include "webgpu/webgpu_cpp.h"

// std
#include <filesystem>
#include <optional>
#include <string>

namespace C2Lux {

struct ObjLoadResult {
    std::optional<Mesh> mesh;
    std::string warning;
    std::string error;
};

class ResourceManager {
   public:
    static ResourceManager& getInstance() {
        static ResourceManager instance;
        return instance;
    }

    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;

    struct VertexHasher {
        size_t operator()(const VertexAttributes& v) const {
            return std::hash<float>()(v.position.x) ^
                   (std::hash<float>()(v.position.y) << 1) ^
                   (std::hash<float>()(v.position.z) << 2);
        }
    };

    ObjLoadResult loadObj(std::string_view objFileName);
    wgpu::ShaderModule loadShaderModule(std::string_view shaderName,
                                        wgpu::Device device);

   private:
    ResourceManager(std::filesystem::path shaderDirectory = SHADER_DIR,
                    std::filesystem::path objDirectory = RESOURCE_DIR);

    std::filesystem::path m_shaderDirectory;
    std::filesystem::path m_objDirectory;

   public:
    void setShaderPath(std::filesystem::path shaderDirectory) {
        m_shaderDirectory = shaderDirectory;
    }
    void setObjPath(std::filesystem::path objDirectory) {
        m_objDirectory = objDirectory;
    }
};

}  // namespace C2Lux
