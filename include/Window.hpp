#pragma once

#include "GLFW/glfw3.h"

// std
#include <string>
#include <memory>

namespace C2Lux {

class Window {
public:
    ~Window();
    Window(const Window& other) = delete;
    Window& operator=(const Window& other) = delete;

    Window(Window&& other) noexcept;
    Window& operator=(Window&& other) noexcept;

    static std::unique_ptr<Window> create(int width, int height, std::string title);
    bool shouldClose();
    void pollEvents();
    void toggleFullscreen();

    float getAndResetScrollDelta();

private:
    Window(GLFWwindow* window);

    GLFWwindow* m_window;
    int m_height;
    int m_width;

    bool m_isFullscreen = false;
    int m_savedX = 0, m_savedY = 0;
    int m_savedWidth = 0, m_savedHeight = 0;

    bool m_framebufferResized = false;
    std::string m_title;
    float m_scrollDelta = 0.0f;
   

public:
    GLFWwindow* getGLFWwindow() const { return m_window; }
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }

    bool hasResized() const { return m_framebufferResized; }
    void setResized(bool resized) { m_framebufferResized = resized; }
    void setScrollDelta(float delta) { m_scrollDelta = delta; }

    std::pair<int, int> getFramebufferSize();

};

} // namespace C2Lux
