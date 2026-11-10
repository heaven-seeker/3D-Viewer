#pragma once

#include <cstdint>
#include <glm/glm.hpp>
#include <vector>

namespace C2Lux {
struct SceneObject {
    glm::mat4 model{1.0f};
    uint32_t meshId;
};

struct Scene {
    std::vector<SceneObject> objects;
};

}  // namespace C2Lux