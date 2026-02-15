#pragma once

#include "Quelos/Core/Window.h"

class SDL_Window;

namespace Quelos {
    class SDLWindow : public Window {
    public:
        explicit SDLWindow(WindowSpecification windowSpecs);

        void Init() override;
        void Shutdown() override;

        void PollEvents() override;

        void SetCursorMode(CursorMode cursorMode) override;

        uint32_t GetWidth() const override { return m_Specifications.Width; }
        uint32_t GetHeight() const override { return m_Specifications.Height; }

        WindowingBackend GetWindowBacked() const override { return WindowingBackend::SDL; }

        bool IsWayland() const override { return m_IsWayland; }

        void* GetWindowHandle() const override { return m_SDLWindow; }
        void* GetNativeWindow() const override { return m_NativeWindowHandle; }
        void* GetNativeDisplay() const override { return m_DisplayHandle; }

        bool ShouldClose() const override { return m_ShouldClose; }
    private:
        WindowSpecification m_Specifications;
        bool m_ShouldClose = false;

        bool m_IsWayland = false;

        SDL_Window* m_SDLWindow = nullptr;

        void* m_NativeWindowHandle = nullptr;
        void* m_DisplayHandle = nullptr;
    };
}
