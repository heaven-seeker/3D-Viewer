#pragma once
#include "glm/glm.hpp"

namespace C2Lux {

class Camera {
public:
    Camera();

    void update();
    void rotate(float deltaX, float deltaY);
    void zoom(float delta);

    void moveForward(float deltaTime);
    void moveBackward(float deltaTime);
    void moveLeft(float deltaTime);
    void moveRight(float deltaTime);

    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix(float aspect) const;

    void debug() const;

private:
    glm::vec3 m_position;
    glm::vec3 m_front;
    glm::vec3 m_up;
    glm::vec3 m_right;
    glm::vec3 m_worldUp;

    float m_yawDegrees;
    float m_pitchDegrees;

    float m_movementSpeed;
    float m_mouseSensitivity;
    float m_fovDegrees;
public:
    glm::vec3 getPosition() const { return m_position; }
};

} // namespace C2Lux