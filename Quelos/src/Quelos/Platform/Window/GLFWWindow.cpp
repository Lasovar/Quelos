#include "qspch.h"
#include "GLFWWindow.h"

#include "Quelos/Core/Application.h"
#include "Quelos/Core/Events/InputEvents.h"
#include "Quelos/Core/Events/WindowEvents.h"

#ifdef QUELOS_PLATFORM_WINDOWS
#define GLFW_EXPOSE_NATIVE_WIN32
#endif

#ifdef QUELOS_PLATFORM_LINUX
#define GLFW_EXPOSE_NATIVE_X11
#define GLFW_EXPOSE_NATIVE_WAYLAND
#endif

#define GLFW_INCLUDE_NONE

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

namespace Quelos {
    static bool s_GLFWInitialized = false;

    static void GLFWErrorCallback(int error, const char* description) {
        QS_CORE_ERROR_TAG("GLFW", "GLFW Error ({0}): {1}", error, description);
    }

    GLFWWindow::GLFWWindow(WindowSpecification windowSpecs)
        : m_Specifications(std::move(windowSpecs)), m_GLFWWindow(nullptr) {
    }

    void GLFWWindow::Init() {
        QS_CORE_INFO("Initializing GLFW");

        if (!s_GLFWInitialized) {
            const int success = glfwInit();
            QS_CORE_ASSERT(success, "Could not initialize GLFW");
            glfwSetErrorCallback(GLFWErrorCallback);

            s_GLFWInitialized = true;
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        m_GLFWWindow = glfwCreateWindow(
            m_Specifications.Width,
            m_Specifications.Height,
            m_Specifications.Title.c_str(),
            nullptr,
            nullptr
        );

        glfwSetWindowUserPointer(m_GLFWWindow, this);

#ifdef QUELOS_PLATFORM_WINDOWS
        m_WindowHandle = glfwGetWin32Window(m_GLFWWindow);
        m_DisplayHandle = nullptr; //glfwGetWin32Display();
#endif
#ifdef QUELOS_PLATFORM_LINUX
        if (glfwGetPlatform() == GLFW_PLATFORM_WAYLAND) {
            m_DisplayHandle = glfwGetWaylandDisplay();
            m_WindowHandle = glfwGetWaylandWindow(m_GLFWWindow);
        }
        else {
            m_DisplayHandle = glfwGetX11Display();
            m_WindowHandle = reinterpret_cast<void*>(glfwGetX11Window(m_GLFWWindow));
        }
#endif

        glfwSetWindowSizeCallback(m_GLFWWindow, [](GLFWwindow* handle, int width, int height) {
            if (width <= 1 || height <= 1) {
                return;
            }

            GLFWWindow& window = *static_cast<GLFWWindow*>(glfwGetWindowUserPointer(handle));

            window.m_Specifications.Width = width;
            window.m_Specifications.Height = height;

            OnResize(width, height);
        });

        glfwSetKeyCallback(m_GLFWWindow, [](GLFWwindow* window, int key, int scancodes, int action, int mods) {
            auto& app = Application::Get();

            switch (action) {
            case GLFW_PRESS: {
                KeyPressedEvent e(static_cast<KeyCode>(key), false);
                app.RaiseEvent(e);
                break;
            }
            case GLFW_RELEASE: {
                KeyReleasedEvent e(static_cast<KeyCode>(key));
                app.RaiseEvent(e);
                break;
            }
            case GLFW_REPEAT: {
                KeyPressedEvent e(static_cast<KeyCode>(key), true);
                app.RaiseEvent(e);
                break;
            }
            default:
                break;
            }
        });

        glfwSetMouseButtonCallback(m_GLFWWindow, [](GLFWwindow* window, int button, int action, int mods) {
            auto& app = Application::Get();

            switch (action) {
            case GLFW_PRESS: {
                MouseButtonPressedEvent e(static_cast<MouseButton>(button));
                app.RaiseEvent(e);
                break;
            }
            case GLFW_RELEASE: {
                MouseButtonReleasedEvent e(static_cast<MouseButton>(button));
                app.RaiseEvent(e);
                break;
            }
            default:
                break;
            }
        });

        glfwSetScrollCallback(m_GLFWWindow, [](GLFWwindow* window, double xOffset, double yOffset) {
            auto& app = Application::Get();

            MouseScrolledEvent e(static_cast<float>(xOffset), static_cast<float>(yOffset));
            app.RaiseEvent(e);
        });

        glfwSetCursorPosCallback(m_GLFWWindow, [](GLFWwindow* handle, double x, double y) {
            auto& app = Application::Get();
            GLFWWindow& window = *static_cast<GLFWWindow*>(glfwGetWindowUserPointer(handle));

            if (window.m_MouseMoveFirst) {
                window.m_LastX = x;
                window.m_LastY = y;
                window.m_MouseMoveFirst = false;

                MouseMovedEvent e(static_cast<float>(x), static_cast<float>(y), 0, 0);
                app.RaiseEvent(e);
                return;
            }

            const float dx = static_cast<float>(x - window.m_LastX);
            const float dy = static_cast<float>(y - window.m_LastY);

            window.m_LastX = x;
            window.m_LastY = y;

            MouseMovedEvent e(static_cast<float>(x), static_cast<float>(y), dx, dy);
            app.RaiseEvent(e);
        });
    }

    void GLFWWindow::Shutdown() {
        glfwDestroyWindow(m_GLFWWindow);
    }

    void GLFWWindow::PollEvents() {
        glfwPollEvents();
    }

    void GLFWWindow::SetCursorMode(const CursorMode cursorMode) {
        switch (cursorMode) {
        case CursorMode::Normal:
            glfwSetInputMode(m_GLFWWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            break;
        case CursorMode::Locked:
            glfwSetInputMode(m_GLFWWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            m_MouseMoveFirst = true;
            break;
        }
    }

    bool GLFWWindow::IsWayland() const { return glfwGetPlatform() == GLFW_PLATFORM_WAYLAND; }

    bool GLFWWindow::ShouldClose() const { return glfwWindowShouldClose(m_GLFWWindow); }
}
