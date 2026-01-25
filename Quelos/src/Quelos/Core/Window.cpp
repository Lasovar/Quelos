#include "qspch.h"
#include "Window.h"

#include "Application.h"
#include "Events/WindowEvents.h"

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

    Window::Window(WindowSpecification windowSpecs)
        : m_Sepcifications(std::move(windowSpecs)), m_GLFWWindow(nullptr)
    {
    }

    void Window::Init() {
        if (!s_GLFWInitialized) {
            const int success = glfwInit();
            QS_CORE_ASSERT(success, "Could not initialize GLFW");
            glfwSetErrorCallback(GLFWErrorCallback);

            s_GLFWInitialized = true;
        }

        m_Sepcifications.Width = m_Sepcifications.Width;
        m_Sepcifications.Height = m_Sepcifications.Height;

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        m_GLFWWindow = glfwCreateWindow(
            m_Sepcifications.Width,
            m_Sepcifications.Height,
            m_Sepcifications.Title.c_str(),
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
            if (width <= 0 || height <= 0) {
                return;
            }

            Window& window = *((Window*)glfwGetWindowUserPointer(handle));

            window.m_Sepcifications.Width = width;
            window.m_Sepcifications.Height = height;
            
            WindowResizeEvent event(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
            Application::Get().RaiseEvent(event);
        });
    }

    void Window::Update() {
        glfwPollEvents();
    }

    Ref<Window> Window::Create(const WindowSpecification& windowSpecification) {
        return Ref<Window>::Create(windowSpecification);
    }

    bool Window::ShouldClose() const {
        return glfwWindowShouldClose(m_GLFWWindow);
    }
}
