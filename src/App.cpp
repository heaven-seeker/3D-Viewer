#include "App.hpp"

#include "ResourceManager.hpp"
#include "glfw3webgpu.h"
#include "imgui.h"
#include "nfd.h"

// std
#include <iomanip>
#include <iostream>

void printMatrix(const std::string& name, const glm::mat4& m) {
    std::cout << "=== " << name << " ===" << std::endl;
    for (int row = 0; row < 4; ++row) {
        std::cout << "[ ";
        for (int col = 0; col < 4; ++col) {
            std::cout << std::setw(8) << std::fixed << std::setprecision(2)
                      << m[col][row] << " ";
        }
        std::cout << "]" << std::endl;
    }
    std::cout << "==================\n" << std::endl;
}

namespace C2Lux {

App::~App() { m_uiLayer.destroy(); }

App::App(std::unique_ptr<Window> window)
    : m_mainWindow(std::move(window)),
      m_appState(AppState::Startup),
      m_isRunning(true) {}

bool App::initWebGPU() {
    if (!m_renderer.initInstance()) return false;
    if (!m_renderer.setSurface(glfwGetWGPUSurface(
            m_renderer.getInstance().Get(), m_mainWindow->getGLFWwindow())))
        return false;
    if (!m_renderer.initAdapter()) return false;
    if (!m_renderer.initDevice()) return false;
    if (!m_renderer.initQueue()) return false;

    const auto [width, height] = m_mainWindow->getFramebufferSize();
    if (!m_renderer.setupSurfaceConfig(width, height)) return false;

    wgpu::ShaderModule shader = ResourceManager::getInstance().loadShaderModule(
        "shader.wgsl", m_renderer.getDevice());
    if (!shader) return false;

    m_renderer.initBuffers();
    m_renderer.initBindGroups();
    m_renderer.initRenderPipeline(shader);

    if (!m_uiLayer.init(m_mainWindow->getGLFWwindow(), m_renderer.getDevice(),
                        m_renderer.getSurfaceFormat())) {
        std::cerr << "Could not initialize UI Layer!" << std::endl;
        return false;
    }

    return true;
}

// TODO: add new class to process input
void App::processInput(float deltaTime) {
    ImGuiIO& io = ImGui::GetIO();

    if (!io.WantCaptureKeyboard) {
        if (glfwGetKey(m_mainWindow->getGLFWwindow(), GLFW_KEY_W) == GLFW_PRESS)
            m_camera.moveForward(deltaTime);
        if (glfwGetKey(m_mainWindow->getGLFWwindow(), GLFW_KEY_S) == GLFW_PRESS)
            m_camera.moveBackward(deltaTime);
        if (glfwGetKey(m_mainWindow->getGLFWwindow(), GLFW_KEY_A) == GLFW_PRESS)
            m_camera.moveLeft(deltaTime);
        if (glfwGetKey(m_mainWindow->getGLFWwindow(), GLFW_KEY_D) == GLFW_PRESS)
            m_camera.moveRight(deltaTime);
        if (glfwGetKey(m_mainWindow->getGLFWwindow(), GLFW_KEY_P) ==
            GLFW_PRESS) {
            printMatrix("\nView Matrix", m_camera.getViewMatrix());
            printMatrix("Projection",
                        m_camera.getProjectionMatrix(1920.0f / 1080.0f));
        }

        static bool f11WasPressed = false;
        bool f11IsPressed = glfwGetKey(m_mainWindow->getGLFWwindow(),
                                       GLFW_KEY_F11) == GLFW_PRESS;

        if (f11IsPressed && !f11WasPressed) {
            m_mainWindow->toggleFullscreen();
        }
        f11WasPressed = f11IsPressed;
    }

    if (!io.WantCaptureMouse) {
        float scroll = m_mainWindow->getAndResetScrollDelta();
        if (scroll != 0.0f) {
            m_camera.zoom(scroll);
        }

        static double lastX = 0.0, lastY = 0.0;
        static bool isDragging = false;

        if (glfwGetMouseButton(m_mainWindow->getGLFWwindow(),
                               GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
            double xpos, ypos;
            glfwGetCursorPos(m_mainWindow->getGLFWwindow(), &xpos, &ypos);

            if (!isDragging) {
                lastX = xpos;
                lastY = ypos;
                isDragging = true;
            } else {
                m_camera.rotate(static_cast<float>(xpos - lastX),
                                static_cast<float>(ypos - lastY));
                lastX = xpos;
                lastY = ypos;
            }
        } else {
            isDragging = false;
        }
    }
}

void App::renderFrame() {
    float deltaTime = getDeltaTime();

    if (m_uiLayer.needsRebuild()) {
        m_uiLayer.rebuildFontsAndScale();
    }

    if (m_uiLayer.m_exitToMenuRequested) {
        m_appState = AppState::Startup;
        m_uiLayer.m_exitToMenuRequested = false;
    }

    if (m_uiLayer.m_exitRequested) {
        m_isRunning = false;
    }

    m_uiLayer.beginFrame();

    switch (m_appState) {
        case AppState::Startup: {
            std::string path = m_uiLayer.buildStartupUI();
            if (!path.empty()) {
                auto result = ResourceManager::getInstance().loadObj(path);

                if (!result.mesh) {
                    std::cerr << result.error << '\n';
                } else {
                    m_appState = AppState::Viewing;
                    m_currentMesh = std::move(*result.mesh);
                    m_renderer.updateMeshBuffers(m_currentMesh);
                }
            }
            m_renderer.drawJustUI(m_uiLayer);
            break;
        }

        case AppState::Viewing: {
            processInput(deltaTime);

            std::string path = m_uiLayer.buildUI();
            if (!path.empty()) {
                auto result = ResourceManager::getInstance().loadObj(path);

                if (!result.mesh) {
                    std::cerr << result.error << '\n';
                } else {
                    m_currentMesh = std::move(*result.mesh);
                    m_renderer.updateMeshBuffers(m_currentMesh);
                }
            }
            m_renderer.draw(m_uiLayer, m_camera, m_currentMesh);
            break;
        }
    }
}

std::unique_ptr<App> App::create() {
    if (!glfwInit()) {
        std::cerr << "Could not initialize GLFW!" << std::endl;
        return nullptr;
    }
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    auto window = Window::create(1920, 1080, "Main window");
    if (!window) {
        std::cerr << "Could not create the main window!" << std::endl;
        glfwTerminate();
        return nullptr;
    }
    return std::make_unique<App>(std::move(window));
}

bool App::isRunning() {
    return m_isRunning && !m_renderer.hasFailed() &&
           !m_mainWindow->shouldClose();
}

void App::mainLoop() {
    m_renderer.processEvents();
    if (m_renderer.hasFailed()) return;
    if (!handleWindowEvents()) return;
    renderFrame();
}

bool App::handleWindowEvents() {
    m_mainWindow->pollEvents();
    auto [width, height] = m_mainWindow->getFramebufferSize();
    if (width <= 0 || height <= 0) {
        glfwWaitEventsTimeout(0.05);
        return false;
    }

    if (m_mainWindow->hasResized()) {
        m_renderer.resizeSurface(width, height);
        m_mainWindow->setResized(false);
    }

    return true;
}

float App::getDeltaTime() const {
    static double lastFrame = 0.0;
    double currentFrame = glfwGetTime();
    float deltaTime = static_cast<float>(currentFrame - lastFrame);
    lastFrame = currentFrame;

    return deltaTime;
}

}  // namespace C2Lux
