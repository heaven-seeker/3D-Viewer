#include "Camera.hpp"
#include "glm/ext.hpp"

// std
#include <iostream>
#include <algorithm>


namespace C2Lux {

Camera::Camera() :
    m_position(0.0f, 0.0f, 5.0f),
    m_worldUp(0.0f, 1.0f, 0.0f),
    m_yawDegrees(-90.0f),

    m_pitchDegrees(0.0f),
    m_movementSpeed(2.5f),
    m_mouseSensitivity(0.1f),
    m_fovDegrees(45.0f)
{
    update();
}

void Camera::update() {
    glm::vec3 newFront;
    newFront.x = cos(glm::radians(m_yawDegrees)) * cos(glm::radians(m_pitchDegrees));
    newFront.y = sin(glm::radians(m_pitchDegrees));
    newFront.z = sin(glm::radians(m_yawDegrees)) * cos(glm::radians(m_pitchDegrees));
    m_front = glm::normalize(newFront);

    m_right = glm::normalize(glm::cross(m_front, m_worldUp));
    m_up = glm::normalize(glm::cross(m_right, m_front));
}

void Camera::rotate(float deltaX, float deltaY) {
    m_yawDegrees += deltaX * m_mouseSensitivity;
    m_pitchDegrees -= deltaY * m_mouseSensitivity;

    m_pitchDegrees = std::clamp(m_pitchDegrees, -89.0f, 89.0f);
    
    update();
}

void Camera::zoom(float delta) {
    m_fovDegrees -= delta * 2.0f;
    m_fovDegrees = std::clamp(m_fovDegrees, 1.0f, 45.0f);
}

void Camera::moveForward(float deltaTime) { m_position += m_front * m_movementSpeed * deltaTime; }
void Camera::moveBackward(float deltaTime) { m_position -= m_front * m_movementSpeed * deltaTime; }
void Camera::moveLeft(float deltaTime) { m_position -= m_right * m_movementSpeed * deltaTime; }
void Camera::moveRight(float deltaTime) { m_position += m_right * m_movementSpeed * deltaTime; }

glm::mat4 Camera::getViewMatrix() const {
    return glm::lookAt(m_position, m_position + m_front, m_up);
}

glm::mat4 Camera::getProjectionMatrix(float aspect) const {
    return glm::perspective(glm::radians(m_fovDegrees), aspect, 0.01f, 100.0f);
}

void Camera::debug() const {
    std::cout << "\r"
        << "Pos: [" << m_position.x << ", " << m_position.y << ", " << m_position.z << "] | "
        << "Front: [" << m_front.x << ", " << m_front.y << ", " << m_front.z << "] | "
        << "Yaw: " << m_yawDegrees << "  Pitch: " << m_pitchDegrees
        << std::string(10, ' ')
        << std::flush;
}

} // namespace C2Lux