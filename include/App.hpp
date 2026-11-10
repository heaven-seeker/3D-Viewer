#pragma once

#include "Camera.hpp"
#include "Mesh.hpp"
#include "Renderer.hpp"
#include "UILayer.hpp"
#include "Window.hpp"

// std
#include <cstdint>
#include <memory>
#include <optional>

namespace C2Lux {

class App {
   public:
    enum class AppState { Startup, Viewing };

    ~App();
    App(std::unique_ptr<Window> window);

    static std::unique_ptr<App> create();
    bool initWebGPU();
    bool isRunning();
    void mainLoop();

    bool handleWindowEvents();

   private:
    std::unique_ptr<Window> m_mainWindow;
    Renderer m_renderer;
    UILayer m_uiLayer;
    Camera m_camera;
    AppState m_appState;

    bool m_isRunning;

    void processInput(float deltaTime);
    void renderFrame();

   public:
    float getDeltaTime() const;

    AppState getAppState() const { return m_appState; }
    void setAppState(AppState newState) { m_appState = newState; }

    std::optional<uint32_t> m_currentMeshId;
};

}  // namespace C2Lux
