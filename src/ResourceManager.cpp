#include "ResourceManager.hpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#define STB_IMAGE_IMPLEMENTATION
#include "std_image.h"

// std
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>

bool operator==(const VertexAttributes& a, const VertexAttributes& b) {
    return a.position == b.position && a.normal == b.normal && a.uv == b.uv &&
           a.color == b.color;
}

namespace C2Lux {

ResourceManager::ResourceManager(std::filesystem::path shaderDirectory,
                                 std::filesystem::path objDirectory)
    : m_shaderDirectory(shaderDirectory), m_objDirectory(objDirectory) {}

wgpu::ShaderModule ResourceManager::loadShaderModule(
    std::string_view shaderName, wgpu::Device device) {
    const auto path = m_shaderDirectory / shaderName;
    std::ifstream file(path);

    if (!file.is_open()) {
        std::cerr << "Error: Can't open shader code file at: " << path.string()
                  << std::endl;
        return nullptr;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string shaderCode = buffer.str();

    if (shaderCode.empty()) {
        std::cerr << "Shader's code is empty" << std::endl;
        return nullptr;
    }

    wgpu::ShaderSourceWGSL wgslDesc{};
    wgslDesc.code = shaderCode.c_str();

    wgpu::ShaderModuleDescriptor shaderDesc{};
    shaderDesc.nextInChain = &wgslDesc;

    return device.CreateShaderModule(&shaderDesc);
}

ObjLoadResult ResourceManager::loadObj(std::string_view objFileName) {
    const auto path = m_objDirectory / objFileName;
    const std::string objPath = path.string();
    const std::string mtlPath = path.parent_path().string() + "/";

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    std::string warn;
    std::string err;

    const auto fail = [&](std::string message) -> ObjLoadResult {
        ObjLoadResult result{};
        result.error = std::move(message);
        result.warning = warn;
        return result;
    };

    const auto validIndex = [](int index, size_t dataSize, size_t components) {
        return index >= 0 && static_cast<size_t>(index) < dataSize / components;
    };

    // Parse and triangulate OBJ
    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err,
                          objPath.c_str(), mtlPath.c_str(), true)) {
        return fail(err.empty() ? "Failed to load OBJ" : err);
    }

    std::vector<VertexAttributes> vertexData;
    std::vector<uint32_t> indexData;
    std::vector<SubMesh> subMeshes;

    std::unordered_map<VertexAttributes, uint32_t, VertexHasher> uniqueVertices;

    for (const auto& shape : shapes) {
        const auto& mesh = shape.mesh;
        const size_t faceCount = mesh.num_face_vertices.size();

        // Validate face array sizes
        if (mesh.material_ids.size() != faceCount ||
            mesh.indices.size() / 3 != faceCount ||
            mesh.indices.size() % 3 != 0) {
            return fail("Invalid OBJ mesh structure");
        }

        // Ensure indices fit into uint32_t
        if (mesh.indices.size() >
            std::numeric_limits<uint32_t>::max() - indexData.size()) {
            return fail("Too many indices");
        }

        int currentMaterialId = -1;
        uint32_t startIndex = static_cast<uint32_t>(indexData.size());
        uint32_t indexCount = 0;

        for (size_t f = 0; f < faceCount; ++f) {
            if (mesh.num_face_vertices[f] != 3) {
                return fail("Expected triangulated mesh");
            }

            const int materialId = mesh.material_ids[f];

            if (materialId < -1 ||
                (materialId >= 0 &&
                 static_cast<size_t>(materialId) >= materials.size())) {
                return fail("Invalid material ID");
            }

            // Start a new submesh when the material changes
            if (materialId != currentMaterialId && indexCount > 0) {
                subMeshes.push_back(
                    {startIndex, indexCount, currentMaterialId});

                startIndex = static_cast<uint32_t>(indexData.size());
                indexCount = 0;
            }

            currentMaterialId = materialId;

            for (size_t v = 0; v < 3; ++v) {
                const auto& idx = mesh.indices[f * 3 + v];

                if (!validIndex(idx.vertex_index, attrib.vertices.size(), 3)) {
                    return fail("Invalid vertex index");
                }

                if (idx.normal_index != -1 &&
                    !validIndex(idx.normal_index, attrib.normals.size(), 3)) {
                    return fail("Invalid normal index");
                }

                if (idx.texcoord_index != -1 &&
                    !validIndex(idx.texcoord_index, attrib.texcoords.size(),
                                2)) {
                    return fail("Invalid texcoord index");
                }

                if (!attrib.colors.empty() &&
                    !validIndex(idx.vertex_index, attrib.colors.size(), 3)) {
                    return fail("Invalid vertex color index");
                }

                VertexAttributes vertex{};

                const size_t p = static_cast<size_t>(idx.vertex_index) * 3;

                vertex.position = {attrib.vertices[p], attrib.vertices[p + 1],
                                   -attrib.vertices[p + 2]};

                if (idx.normal_index >= 0) {
                    const size_t n = static_cast<size_t>(idx.normal_index) * 3;

                    vertex.normal = {attrib.normals[n], attrib.normals[n + 1],
                                     -attrib.normals[n + 2]};
                }

                if (idx.texcoord_index >= 0) {
                    const size_t t =
                        static_cast<size_t>(idx.texcoord_index) * 2;

                    vertex.uv = {attrib.texcoords[t],
                                 1.0f - attrib.texcoords[t + 1]};
                }

                if (!attrib.colors.empty()) {
                    vertex.color = {attrib.colors[p], attrib.colors[p + 1],
                                    attrib.colors[p + 2]};
                } else {
                    vertex.color = {1.0f, 1.0f, 1.0f};
                }

                // Reuse existing vertices
                auto [it, inserted] = uniqueVertices.try_emplace(
                    vertex, static_cast<uint32_t>(vertexData.size()));

                if (inserted) {
                    vertexData.push_back(vertex);
                }

                indexData.push_back(it->second);
                ++indexCount;
            }
        }

        if (indexCount > 0) {
            subMeshes.push_back({startIndex, indexCount, currentMaterialId});
        }
    }

    if (vertexData.empty() || indexData.empty()) {
        return fail("OBJ contains no renderable geometry");
    }

    std::vector<Material> parsedMaterials;
    parsedMaterials.reserve(materials.size());

    for (const auto& tinyMat : materials) {
        Material material{};

        material.name = tinyMat.name;
        material.diffuseColor = {tinyMat.diffuse[0], tinyMat.diffuse[1],
                                 tinyMat.diffuse[2]};

        parsedMaterials.push_back(std::move(material));
    }

    ObjLoadResult result{};

    result.mesh.emplace(std::move(vertexData), std::move(indexData),
                        std::move(subMeshes), std::move(parsedMaterials));

    result.warning = std::move(warn);
    return result;
}

}  // namespace C2Lux
