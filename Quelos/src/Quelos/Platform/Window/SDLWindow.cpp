#include "qspch.h"
#include "SDLWindow.h"

#include "SDL3/SDL.h"

#include "Quelos/ImGui/ImGuiLayer.h"
#include "Quelos/ImGui/imgui_impl_sdl3.h"

namespace Quelos {
    SDLWindow::SDLWindow(WindowSpecification windowSpecs) :
        m_Specifications(std::move(windowSpecs)) {
    }

    void SDLWindow::Init() {
        SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);

        m_SDLWindow = SDL_CreateWindow(
            m_Specifications.Title.c_str(),
            m_Specifications.Width,
            m_Specifications.Height,
            0
        );

        const SDL_PropertiesID windowProperties = SDL_GetWindowProperties(m_SDLWindow);

#if QUELOS_PLATFORM_WINDOWS
        m_NativeWindowHandle = SDL_GetPointerProperty(
            windowProperties,
            SDL_PROP_WINDOW_WIN32_HWND_POINTER,
            nullptr
        );

        m_DisplayHandle = nullptr;
#elif QUELOS_PLATFORM_LINUX
        m_IsWayland = SDL_GetPointerProperty(
            SDL_GetWindowProperties(m_SDLWindow),
            SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER,
            nullptr
        );

        if (m_IsWayland) {
            m_NativeWindowHandle = SDL_GetPointerProperty(
                windowProperties,
                SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER,
                nullptr
            );

            m_DisplayHandle = SDL_GetPointerProperty(
                SDL_GetWindowProperties(m_SDLWindow),
                SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER,
                nullptr
            );
        } else {
            m_NativeWindowHandle = SDL_GetPointerProperty(
                windowProperties,
                SDL_PROP_WINDOW_X11_WINDOW_NUMBER,
                nullptr
            );

            m_DisplayHandle = SDL_GetPointerProperty(
                SDL_GetWindowProperties(m_SDLWindow),
                SDL_PROP_WINDOW_X11_DISPLAY_POINTER,
                nullptr
            );
        }
#endif
    }

    void SDLWindow::Shutdown() {
        SDL_DestroyWindow(m_SDLWindow);
        SDL_Quit();
    }

    void SDLWindow::PollEvents() {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_EVENT_QUIT:
                m_ShouldClose = true;
                break;
            case SDL_EVENT_WINDOW_RESIZED:
                m_Specifications.Width = event.window.data1;
                m_Specifications.Height = event.window.data2;

                OnResize(m_Specifications.Width, m_Specifications.Height);
                break;
            default:
                break;
            }

		    ImGui_ImplSDL3_ProcessEvent(&event);
        }
    }
}
