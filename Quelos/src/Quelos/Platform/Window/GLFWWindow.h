#pragma once

#include "Quelos/Core/Event.h"
#include "Quelos/Core/Window.h"

class GLFWwindow;

namespace Quelos {

    class GLFWWindow : public Window {
    public:
        using EventCallbackFn = std::function<void(Event&)>;

        explicit GLFWWindow(WindowSpecification windowSpecs);

        void Init() override;
        void Shutdown() override;

        void PollEvents() override;

        uint32_t GetWidth() const override { return m_Specifications.Width; }
        uint32_t GetHeight() const override { return m_Specifications.Height; }

        WindowingBackend GetWindowBacked() const override { return WindowingBackend::GLFW; }

        bool IsWayland() const override;

        void* GetWindowHandle() const override { return m_GLFWWindow; }
        void* GetNativeWindow() const override { return m_WindowHandle; }
        void* GetNativeDisplay() const override { return m_DisplayHandle; }

        bool ShouldClose() const override;
    private:
        WindowSpecification m_Specifications;
        GLFWwindow* m_GLFWWindow;
        void* m_WindowHandle = nullptr;
        void* m_DisplayHandle = nullptr;
    };
}
