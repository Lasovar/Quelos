#include "qspch.h"
#include "Window.h"

#include "Application.h"
#include "Events/WindowEvents.h"

// Maybe try to have both linked the same library?
#ifdef QUELOS_USE_SDL
#include "Quelos/Platform/Window/SDLWindow.h"
#elif QUELOS_USE_GLFW
#include "Quelos/Platform/Window/GLFWWindow.h"
#endif

namespace Quelos {
    Ref<Window> Window::Create(const WindowSpecification& windowSpecification) {
        switch (windowSpecification.Backed) {
#ifdef QUELOS_USE_SDL
            case WindowingBackend::SDL:     return CreateRef<SDLWindow>(windowSpecification);
#elif QUELOS_USE_GLFW
            case WindowingBackend::GLFW:    return CreateRef<GLFWWindow>(windowSpecification);
#endif
            default:                        QS_CORE_ASSERT(false, "Unknown Window Backed!");
        }

        return nullptr;
    }

    void Window::OnResize(const uint32_t width, const uint32_t height) {
        WindowResizeEvent event(width, height);
        Application::Get().RaiseEvent(event);
    }
}
