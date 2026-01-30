#pragma once
#if 0

#include "Quelos/Core/Window.h"

#include "gtk/gtk.h"

struct wl_egl_window;

namespace Quelos {
    class GTKWindow : public Window {
    public:
        explicit GTKWindow(WindowSpecification  windowSpecs);

        void Init() override;
        void Shutdown() override;

        void PollEvents() override {}

        uint32_t GetWidth() const override { return m_Specifications.Width; }
        uint32_t GetHeight() const override { return m_Specifications.Height; }

        void* GetWindowHandle() const override { return m_GtkWindow; }
        void* GetNativeWindow() const override { return m_EglWindow; }
        void* GetNativeDisplay() const override { return m_NativeDisplay; }

        bool ShouldClose() const override { return m_ShouldClose; }
        int m_ResizeCount = 0;
        bool m_SurfaceReady = false;
        GtkWidget* m_GtkDrawingArea;
        wl_egl_window* m_EglWindow;

    private:
        WindowSpecification m_Specifications;

        bool m_ShouldClose = false;

        void* m_NativeWindowSurface = nullptr;
        void* m_NativeDisplay = nullptr;
        GtkWidget* m_GtkWindow;
    };
}
#endif
