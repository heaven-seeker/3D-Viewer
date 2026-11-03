#include "Mesh.hpp"

namespace C2Lux {

Mesh::Mesh(std::vector<VertexAttributes> vertices,
           std::vector<uint32_t> indices, std::vector<SubMesh> subMeshes,
           std::vector<Material> materials)
    : m_vertexData(std::move(vertices)),
      m_indexData(std::move(indices)),
      m_subMeshes(std::move(subMeshes)),
      m_materials(std::move(materials)) {}

bool Mesh::setMaterialColor(uint32_t materialId, const glm::vec3& color) {
    if (materialId >= m_materials.size()) {
        return false;
    }

    m_materials[materialId].diffuseColor = color;
    return true;
}

}  // namespace C2Lux
