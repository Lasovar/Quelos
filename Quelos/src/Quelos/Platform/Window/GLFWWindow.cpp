#include "qspch.h"
#include "GLFWWindow.h"

#include "Quelos/Core/Application.h"
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
        : m_Specifications(std::move(windowSpecs)), m_GLFWWindow(nullptr)
    {
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
    }

    void GLFWWindow::Shutdown() {
        glfwDestroyWindow(m_GLFWWindow);
    }

    void GLFWWindow::PollEvents() {
        glfwPollEvents();
    }

    bool GLFWWindow::IsWayland() const { return glfwGetPlatform() == GLFW_PLATFORM_WAYLAND; }

    bool GLFWWindow::ShouldClose() const { return glfwWindowShouldClose(m_GLFWWindow); }
}
