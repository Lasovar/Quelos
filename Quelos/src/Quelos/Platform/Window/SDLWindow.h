#pragma once

#include "Quelos/Core/Window.h"

#include "SDL3/SDL.h"

namespace Quelos {
    class SDLWindow : public Window {
    public:
        explicit SDLWindow(WindowSpecification windowSpecs);

        void Init() override;
        void Shutdown() override;

        void PollEvents() override;

        uint32_t GetWidth() const override { return m_Specifications.Width; }
        uint32_t GetHeight() const override { return m_Specifications.Height; }

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
