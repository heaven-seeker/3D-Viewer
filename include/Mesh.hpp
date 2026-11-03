#pragma once

#include "glm/glm.hpp"

// std
#include <string>
#include <vector>

struct VertexAttributes {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 color;
    glm::vec2 uv;
};

struct SubMesh {
    uint32_t startIndex;
    ;
    uint32_t indexCount;
    int materialId;
};

struct Material {
    std::string name;
    glm::vec3 diffuseColor{1.0f, 1.0f, 1.0f};
    std::string diffuseTexture;
};

namespace C2Lux {

class Mesh {
   public:
    Mesh() = default;

    Mesh(std::vector<VertexAttributes> vertices, std::vector<uint32_t> indices,
         std::vector<SubMesh> subMeshes, std::vector<Material> materials);

   private:
    std::vector<VertexAttributes> m_vertexData;
    std::vector<uint32_t> m_indexData;
    std::vector<SubMesh> m_subMeshes;
    std::vector<Material> m_materials;

   public:
    const std::vector<VertexAttributes>& getVertexData() const {
        return m_vertexData;
    }
    const std::vector<uint32_t>& getIndexData() const { return m_indexData; }
    const std::vector<SubMesh>& getSubMeshes() const { return m_subMeshes; }
    const std::vector<Material>& getMaterials() const { return m_materials; }
};

}  // namespace C2Lux
