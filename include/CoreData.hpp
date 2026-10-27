#pragma once

#include "glm/glm.hpp"

inline constexpr float PI = 3.14159265358979323846;

struct Uniforms {
    glm::mat4 modelMatrix;
    glm::mat4 viewMatrix;
    glm::mat4 projectionMatrix;
    glm::vec4 color;
    glm::vec3 cameraPosition;
    float time;
};

struct LightData {
    glm::vec3 direction;
    float ambient;
    float intensity;
    float shininess;
    float specularStrength;
    float _pad;
};

// Only numeric GPU data belongs here; Material keeps the CPU-side metadata.
struct alignas(16) MaterialUniforms {
    glm::vec4 baseColor{1.0f};
};

static_assert(sizeof(Uniforms) % 16 == 0);
static_assert(sizeof(LightData) % 16 == 0);
static_assert(sizeof(MaterialUniforms) == 16);
