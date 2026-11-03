#pragma once

#include "glm/glm.hpp"

#include <cstddef>

inline constexpr float PI = 3.14159265358979323846;

struct Uniforms {
    glm::mat4 modelMatrix;
    glm::mat4 viewMatrix;
    glm::mat4 projectionMatrix;
    glm::vec4 color;
    glm::vec3 cameraPosition;
    float time;
};

struct alignas(16) LightData {
    glm::vec3 direction;
    float ambient;
    float intensity;
    float _pad[3]{};
};

// Only numeric GPU data belongs here; Material keeps the CPU-side metadata.
struct alignas(16) MaterialUniforms {
    glm::vec4 baseColor{1.0f};

    glm::vec3 specularColor{1.0f};
    float specularStrength = 0.5f;

    float shininess = 32.f;

    float _pad[3]{};
};

static_assert(sizeof(Uniforms) % 16 == 0);
static_assert(sizeof(LightData) == 32);
static_assert(offsetof(LightData, intensity) == 16);
static_assert(sizeof(MaterialUniforms) == 48);
static_assert(offsetof(MaterialUniforms, specularColor) == 16);
static_assert(offsetof(MaterialUniforms, specularStrength) == 28);
static_assert(offsetof(MaterialUniforms, shininess) == 32);
