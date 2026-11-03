#pragma once

#include "webgpu/webgpu_cpp.h"
#include "GLFW/glfw3.h"
#include "glm/glm.hpp"

// std
#include <string>

namespace C2Lux {

class UILayer {
public:
    UILayer() = default;
    ~UILayer();

    bool init(GLFWwindow* window, wgpu::Device device, wgpu::TextureFormat targetFormat);

    void destroy();

    void beginFrame();
    void endFrame();

    std::string buildUI();
    std::string buildStartupUI();
    void rebuildFontsAndScale();
    void buildSettingsBlock();

    void draw(wgpu::RenderPassEncoder renderPass);

    glm::vec4 m_backgroundColor = { 0.1f, 0.1f, 0.1f, 1.0f };
    glm::vec4 m_modelColor = { 0.8f, 0.8f, 0.8f, 1.0f };

    // Light data
    float m_lightDirection[3] = { 1.0f, 1.0f, 1.0f };
    float m_ambientStrength = 0.1f;
    float m_lightIntensity = 0.2f;
    float m_shininess = 10.f;
    float m_specularStrength = 0.1f;

    bool m_fullscreen = false;
    bool m_exitToMenuRequested = false;
    bool m_exitRequested = false;
    bool m_fullscreenFlag = false;

private:
    bool m_nfdInitialized = false;
    bool m_glfwInitialized = false;
    bool m_webgpuInitialized = false;

    unsigned int m_fontSize = 16.f;
    float m_uiScale = 2.1f;
    bool m_needsRebuild = false;

public:
    bool needsRebuild() const { return m_needsRebuild; }
};

} // namespace C2Lux
