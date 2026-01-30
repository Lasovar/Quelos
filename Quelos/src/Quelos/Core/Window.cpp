#include "qspch.h"
#include "Window.h"

#include "Application.h"
#include "Events/WindowEvents.h"
#include "Quelos/Platform/Window/GLFWWindow.h"
#include "Quelos/Platform/Window/SDLWindow.h"

namespace Quelos {
    Ref<Window> Window::Create(const WindowSpecification& windowSpecification) {
        return CreateRef<SDLWindow>(windowSpecification);
    }

    void Window::OnResize(const uint32_t width, const uint32_t height) {
        WindowResizeEvent event(width, height);
        Application::Get().RaiseEvent(event);
    }
}
