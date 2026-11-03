#include "Window.hpp"
#include "GLFW/glfw3.h"


namespace C2Lux {

Window::Window(GLFWwindow* window) {
    m_window = window;
}
    
Window::~Window() {
    glfwDestroyWindow(m_window);
    
}

Window::Window(Window&& other) noexcept {
    if (this != &other) {
        m_window = other.m_window;
        m_height = other.m_height;
        m_width = other.m_width;
        m_title = other.m_title;

        other.m_window = nullptr;
        other.m_height = -1;
        other.m_width = -1;
        other.m_title = "";
    }
}

Window& Window::operator=(Window&& other) noexcept {
    if (this != &other) {
        glfwDestroyWindow(m_window);
        m_window = other.m_window;
        m_height = other.m_height;
        m_width = other.m_width;
        m_title = other.m_title;

        other.m_window = nullptr;
        other.m_height = -1;
        other.m_width = -1;
        other.m_title = "";
    }
    return *this;
}

std::unique_ptr<Window> Window::create(int width, int height, std::string title) {
    //glfwWindowHintString(GLFW_WAYLAND_APP_ID, "c2lux.viewer");

    GLFWwindow* glfwWindow = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!glfwWindow) {
        return nullptr;
    } else {
        std::unique_ptr<Window> wPtr(new Window(glfwWindow));
        glfwSetWindowUserPointer(glfwWindow, wPtr.get());
        glfwSetFramebufferSizeCallback(
            wPtr->getGLFWwindow(), 
            [](GLFWwindow* window, int, int) {
                auto that = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
                if (that != nullptr) that->setResized(true);
            }
        );
        glfwSetScrollCallback(
            wPtr->getGLFWwindow(),
            [](GLFWwindow* window, double xoffset, double yoffset) {
                auto that = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
                if (that != nullptr) {
                    that->setScrollDelta(static_cast<float>(yoffset));
                }
            }
        );
        return wPtr;
    }
}

void Window::pollEvents() {
    glfwPollEvents();
}

void Window::toggleFullscreen()
{
    if (!m_isFullscreen) {
        glfwGetWindowPos(m_window, &m_savedX, &m_savedY);
        glfwGetWindowSize(m_window, &m_savedWidth, &m_savedHeight);

        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);

        glfwSetWindowMonitor(m_window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
        m_isFullscreen = true;
    }
    else {
        glfwSetWindowMonitor(m_window, nullptr, m_savedX, m_savedY, m_savedWidth, m_savedHeight, 0);
        m_isFullscreen = false;
    }
}

float Window::getAndResetScrollDelta()
{
    float d = m_scrollDelta;
    m_scrollDelta = 0.0f;
    return d;   
}

bool Window::shouldClose() {
    return glfwWindowShouldClose(m_window);
}

std::pair<int, int> Window::getFramebufferSize() {
    int framebufferWidth, framebufferHeight;
    glfwGetFramebufferSize(m_window, &framebufferWidth, &framebufferHeight);
    return std::pair<int, int>(framebufferWidth, framebufferHeight);
}

} // namespace C2Lux
