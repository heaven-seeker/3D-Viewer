#include "Mesh.hpp"

namespace C2Lux {

Mesh::Mesh(std::string name, std::vector<VertexAttributes> vertices,
           std::vector<uint32_t> indices, std::vector<SubMesh> subMeshes,
           std::vector<Material> materials)
    : m_name(std::move(name)),
      m_vertexData(std::move(vertices)),
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

bool Mesh::setMaterial(uint32_t materialId, const Material& material) {
    if (materialId >= m_materials.size()) {
        return false;
    }

    m_materials[materialId] = material;
    return true;
}

bool Mesh::validate() const {
    const size_t totalIndices = m_indexData.size();

    for (uint32_t index : m_indexData) {
        if (index >= m_vertexData.size()) {
            return false;
        }
    }

    for (const SubMesh& submesh : m_subMeshes) {
        if (submesh.startIndex > totalIndices) {
            return false;
        }

        if (submesh.indexCount > totalIndices - submesh.startIndex) {
            return false;
        }

        if (submesh.indexCount % 3 != 0) {
            return false;
        }

        if (submesh.materialId < -1) {
            return false;
        }

        if (submesh.materialId >= 0 &&
            static_cast<size_t>(submesh.materialId) >= m_materials.size()) {
            return false;
        }
    }

    return true;
}

}  // namespace C2Lux
