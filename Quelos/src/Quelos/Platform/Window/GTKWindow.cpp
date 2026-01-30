#if 0
#include "qspch.h"
#include "GTKWindow.h"

#include "Quelos/Core/Application.h"

#include <gtk/gtkapplication.h>

#include <gdk/gdk.h>
#include <gdk/wayland/gdkwayland.h>
#include <gdk/x11/gdkx.h>

#include <wayland-egl.h>

#include "Quelos/Renderer/Renderer.h"

// DOESN'T REALLY WORK MIGHT BE REMOVED CAUSE TS ASS

namespace Quelos {
    static GtkApplication* s_GTKApp = nullptr;

    static gboolean OnTick(GtkWidget* widget, GdkFrameClock* clock, const gpointer userData) {

        GTKWindow& window = *static_cast<GTKWindow*>(userData);
        if (!gtk_widget_get_realized(window.m_GtkDrawingArea)) {
            return G_SOURCE_CONTINUE;
        }

        if (!Renderer::IsInitialized()) {
            //if (window.SurfaceReady())
            {Renderer::Init(Ref<Window>(&window));}
        }

        Application::Get().Tick();

        if (window.m_ResizeCount > 1) {
            window.m_SurfaceReady = true;
        }
        return G_SOURCE_CONTINUE;
    }

    GTKWindow::GTKWindow(WindowSpecification windowSpecs)
        : m_Specifications(std::move(windowSpecs)), m_GtkWindow(nullptr)
    {
    }

    void GTKWindow::Init() {
        s_GTKApp = gtk_application_new("com.quelos.editor", G_APPLICATION_DEFAULT_FLAGS);

        g_signal_connect(s_GTKApp, "activate", G_CALLBACK(+[](GtkApplication* app, gpointer userData) {
            const auto window = static_cast<GTKWindow*>(userData);
            window->m_GtkWindow = gtk_application_window_new(app);
            gtk_window_set_title(GTK_WINDOW(window->m_GtkWindow), window->m_Specifications.Title.c_str());
            gtk_window_set_default_size(GTK_WINDOW(window->m_GtkWindow), window->m_Specifications.Width, window->m_Specifications.Height);

            window->m_GtkDrawingArea = gtk_drawing_area_new();

            gtk_drawing_area_set_draw_func(
                GTK_DRAWING_AREA(window->m_GtkDrawingArea),
                [](GtkDrawingArea*, cairo_t*, int, int, gpointer) {},
                nullptr,
                nullptr
            );

            gtk_window_set_child(GTK_WINDOW(window->m_GtkWindow), window->m_GtkDrawingArea);

            gtk_window_present(GTK_WINDOW(window->m_GtkWindow));

            g_signal_connect(
                window->m_GtkDrawingArea,
                "resize",
                G_CALLBACK(+[](GtkDrawingArea* area, int width, int height, gpointer userData) {
                    if (width <= 1 || height <= 1) {
                        return;
                    }

                    const auto window = static_cast<GTKWindow*>(userData);

                    window->m_Specifications.Width = width;
                    window->m_Specifications.Height = height;

                    OnResize(width, height);
                    QS_CORE_INFO("Window resized to: {}x{}", width, height);
                    window->m_ResizeCount++;
                    if (window->m_ResizeCount == 2) {
                        // GTK fires resize event twice, so we only want to handle it once
                        window->m_EglWindow = wl_egl_window_create(static_cast<wl_surface*>(window->GetNativeWindow()), width, height);
                    }
                }),
                window
            );

            /*g_signal_connect(window->m_GtkWindow, "close-request", G_CALLBACK(+[](GtkWindow* _, const gpointer userData) {
                                 const auto window = static_cast<GTKWindow*>(userData);
                                 window->m_ShouldClose = true;
                                 return TRUE;
                                 }), window);*/

            GtkNative* native = gtk_widget_get_native(GTK_WIDGET(window->m_GtkDrawingArea));
            GdkSurface* surface = gtk_native_get_surface(native);

            GdkDisplay* display = gdk_display_get_default();

            if (GDK_IS_WAYLAND_DISPLAY(display)) {
                window->m_NativeWindowSurface = gdk_wayland_surface_get_wl_surface(surface);
                GdkDisplay* surfaceDisplay = gdk_surface_get_display(surface);
                window->m_NativeDisplay = gdk_wayland_display_get_wl_display(surfaceDisplay);
                }
            else if (GDK_IS_X11_DISPLAY(display)) {
                window->m_NativeWindowSurface = reinterpret_cast<void*>(gdk_x11_surface_get_xid(surface));
                window->m_NativeDisplay = gdk_x11_display_get_xdisplay(gdk_surface_get_display(surface));
            }
            else { }

            QS_CORE_INFO("Native Window Handle: {}", window->m_NativeWindowSurface);
            QS_CORE_INFO("Native Display Handle: {}", window->m_NativeDisplay);

            gtk_drawing_area_set_draw_func(
    GTK_DRAWING_AREA(window->m_GtkDrawingArea),
    +[](GtkDrawingArea* area,
        cairo_t*,
        int,
        int,
        gpointer userData)
    {
        auto* window = static_cast<GTKWindow*>(userData);

        QS_CORE_INFO("S");
    },
    window,
    nullptr
);
        gtk_widget_add_tick_callback(window->m_GtkDrawingArea, OnTick, window, nullptr);
        }), this);
    }

    void GTKWindow::Shutdown() {
        gtk_window_destroy(GTK_WINDOW(m_GtkWindow));
    }

    /*
    int GTKWindow::Run() {
        const int status = g_application_run(G_APPLICATION(s_GTKApp), 0, nullptr);
        g_object_unref(s_GTKApp);

        if (status != 0) {
            QS_CORE_ERROR_TAG("GTK", "Could not initialize GTK Application");
        }

        return status;
    }*/
}
#endif
