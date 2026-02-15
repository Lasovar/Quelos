#include "qspch.h"
#include "SDLWindow.h"

#include "Quelos/Core/Application.h"
#include "Quelos/Core/Events/InputEvents.h"
#include "SDL3/SDL.h"

#include "Quelos/ImGui/ImGuiLayer.h"
#include "Quelos/ImGui/imgui_impl_sdl3.h"

namespace Quelos {
    namespace Utils {
        KeyCode SDLKeycodeToKeyCode(const SDL_Keycode key) {
            switch (key) {
            case SDLK_SPACE: return KeyCode::Space;
            case SDLK_APOSTROPHE: return KeyCode::Apostrophe;
            case SDLK_COMMA: return KeyCode::Comma;
            case SDLK_MINUS: return KeyCode::Minus;
            case SDLK_PERIOD: return KeyCode::Period;
            case SDLK_SLASH: return KeyCode::Slash;

            case SDLK_0: return KeyCode::Alpha0;
            case SDLK_1: return KeyCode::Alpha1;
            case SDLK_2: return KeyCode::Alpha2;
            case SDLK_3: return KeyCode::Alpha3;
            case SDLK_4: return KeyCode::Alpha4;
            case SDLK_5: return KeyCode::Alpha5;
            case SDLK_6: return KeyCode::Alpha6;
            case SDLK_7: return KeyCode::Alpha7;
            case SDLK_8: return KeyCode::Alpha8;
            case SDLK_9: return KeyCode::Alpha9;

            case SDLK_SEMICOLON: return KeyCode::Semicolon;
            case SDLK_EQUALS: return KeyCode::Equal;

            case SDLK_A: return KeyCode::A;
            case SDLK_B: return KeyCode::B;
            case SDLK_C: return KeyCode::C;
            case SDLK_D: return KeyCode::D;
            case SDLK_E: return KeyCode::E;
            case SDLK_F: return KeyCode::F;
            case SDLK_G: return KeyCode::G;
            case SDLK_H: return KeyCode::H;
            case SDLK_I: return KeyCode::I;
            case SDLK_J: return KeyCode::J;
            case SDLK_K: return KeyCode::K;
            case SDLK_L: return KeyCode::L;
            case SDLK_M: return KeyCode::M;
            case SDLK_N: return KeyCode::N;
            case SDLK_O: return KeyCode::O;
            case SDLK_P: return KeyCode::P;
            case SDLK_Q: return KeyCode::Q;
            case SDLK_R: return KeyCode::R;
            case SDLK_S: return KeyCode::S;
            case SDLK_T: return KeyCode::T;
            case SDLK_U: return KeyCode::U;
            case SDLK_V: return KeyCode::V;
            case SDLK_W: return KeyCode::W;
            case SDLK_X: return KeyCode::X;
            case SDLK_Y: return KeyCode::Y;
            case SDLK_Z: return KeyCode::Z;

            case SDLK_LEFTBRACKET: return KeyCode::LeftBracket;
            case SDLK_BACKSLASH: return KeyCode::Backslash;
            case SDLK_RIGHTBRACKET: return KeyCode::RightBracket;
            case SDLK_GRAVE: return KeyCode::GraveAccent;

            // Control keys
            case SDLK_ESCAPE: return KeyCode::Escape;
            case SDLK_RETURN: return KeyCode::Enter;
            case SDLK_TAB: return KeyCode::Tab;
            case SDLK_BACKSPACE: return KeyCode::Backspace;
            case SDLK_INSERT: return KeyCode::Insert;
            case SDLK_DELETE: return KeyCode::Delete;

            case SDLK_RIGHT: return KeyCode::Right;
            case SDLK_LEFT: return KeyCode::Left;
            case SDLK_DOWN: return KeyCode::Down;
            case SDLK_UP: return KeyCode::Up;

            case SDLK_PAGEUP: return KeyCode::Page_up;
            case SDLK_PAGEDOWN: return KeyCode::Page_down;
            case SDLK_HOME: return KeyCode::Home;
            case SDLK_END: return KeyCode::End;

            case SDLK_CAPSLOCK: return KeyCode::Caps_lock;
            case SDLK_SCROLLLOCK: return KeyCode::Scroll_lock;
            case SDLK_NUMLOCKCLEAR: return KeyCode::Num_lock;
            case SDLK_PRINTSCREEN: return KeyCode::Print_screen;
            case SDLK_PAUSE: return KeyCode::Pause;

            // Function keys
            case SDLK_F1: return KeyCode::F1;
            case SDLK_F2: return KeyCode::F2;
            case SDLK_F3: return KeyCode::F3;
            case SDLK_F4: return KeyCode::F4;
            case SDLK_F5: return KeyCode::F5;
            case SDLK_F6: return KeyCode::F6;
            case SDLK_F7: return KeyCode::F7;
            case SDLK_F8: return KeyCode::F8;
            case SDLK_F9: return KeyCode::F9;
            case SDLK_F10: return KeyCode::F10;
            case SDLK_F11: return KeyCode::F11;
            case SDLK_F12: return KeyCode::F12;
            case SDLK_F13: return KeyCode::F13;
            case SDLK_F14: return KeyCode::F14;
            case SDLK_F15: return KeyCode::F15;
            case SDLK_F16: return KeyCode::F16;
            case SDLK_F17: return KeyCode::F17;
            case SDLK_F18: return KeyCode::F18;
            case SDLK_F19: return KeyCode::F19;
            case SDLK_F20: return KeyCode::F20;
            case SDLK_F21: return KeyCode::F21;
            case SDLK_F22: return KeyCode::F22;
            case SDLK_F23: return KeyCode::F23;
            case SDLK_F24: return KeyCode::F24;

            // Keypad
            case SDLK_KP_0: return KeyCode::Kp0;
            case SDLK_KP_1: return KeyCode::Kp1;
            case SDLK_KP_2: return KeyCode::Kp2;
            case SDLK_KP_3: return KeyCode::Kp3;
            case SDLK_KP_4: return KeyCode::Kp4;
            case SDLK_KP_5: return KeyCode::Kp5;
            case SDLK_KP_6: return KeyCode::Kp6;
            case SDLK_KP_7: return KeyCode::Kp7;
            case SDLK_KP_8: return KeyCode::Kp8;
            case SDLK_KP_9: return KeyCode::Kp9;

            case SDLK_KP_DECIMAL: return KeyCode::KpDecimal;
            case SDLK_KP_DIVIDE: return KeyCode::KpDivide;
            case SDLK_KP_MULTIPLY: return KeyCode::KpMultiply;
            case SDLK_KP_MINUS: return KeyCode::KpSubtract;
            case SDLK_KP_PLUS: return KeyCode::KpAdd;
            case SDLK_KP_ENTER: return KeyCode::KpEnter;
            case SDLK_KP_EQUALS: return KeyCode::KpEqual;

            // Modifiers
            case SDLK_LSHIFT: return KeyCode::LeftShift;
            case SDLK_LCTRL: return KeyCode::LeftControl;
            case SDLK_LALT: return KeyCode::LeftAlt;
            case SDLK_LGUI: return KeyCode::LeftSuper;

            case SDLK_RSHIFT: return KeyCode::RightShift;
            case SDLK_RCTRL: return KeyCode::RightControl;
            case SDLK_RALT: return KeyCode::RightAlt;
            case SDLK_RGUI: return KeyCode::RightSuper;

            case SDLK_MENU: return KeyCode::Menu;

            default:
                return KeyCode::Last;
            }
        }

        MouseButton SDLMouseButtonCodeToMouseButton(const uint8_t button) {
            switch (button) {
            case SDL_BUTTON_LEFT: return MouseButton::Left;
            case SDL_BUTTON_MIDDLE: return MouseButton::Middle;
            case SDL_BUTTON_RIGHT: return MouseButton::Right;
            case SDL_BUTTON_X1: return MouseButton::X1;
            case SDL_BUTTON_X2: return MouseButton::X2;
            default: return MouseButton::Last;
            }
        }
    }

    SDLWindow::SDLWindow(WindowSpecification windowSpecs) :
        m_Specifications(std::move(windowSpecs)) {
    }

    void SDLWindow::Init() {
        SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);

        m_SDLWindow = SDL_CreateWindow(
            m_Specifications.Title.c_str(),
            m_Specifications.Width,
            m_Specifications.Height,
            SDL_WINDOW_RESIZABLE
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
        }
        else {
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
            case SDL_EVENT_KEY_DOWN: {
                auto& app = Application::Get();
                KeyPressedEvent e(Utils::SDLKeycodeToKeyCode(event.key.key), event.key.repeat);
                app.RaiseEvent(e);
                break;
            }
            case SDL_EVENT_KEY_UP: {
                auto& app = Application::Get();
                KeyReleasedEvent e(Utils::SDLKeycodeToKeyCode(event.key.key));
                app.RaiseEvent(e);
                break;
            }
            case SDL_EVENT_MOUSE_BUTTON_DOWN: {
                auto& app = Application::Get();
                MouseButtonPressedEvent e(Utils::SDLMouseButtonCodeToMouseButton(event.button.button));
                app.RaiseEvent(e);
                break;
            }
            case SDL_EVENT_MOUSE_BUTTON_UP: {
                auto& app = Application::Get();
                MouseButtonReleasedEvent e(Utils::SDLMouseButtonCodeToMouseButton(event.button.button));
                app.RaiseEvent(e);
                break;
            }
            case SDL_EVENT_MOUSE_MOTION: {
                auto& app = Application::Get();
                MouseMovedEvent e(event.motion.x, event.motion.y, event.motion.xrel, event.motion.yrel);
                app.RaiseEvent(e);
                break;
            }
            case SDL_EVENT_MOUSE_WHEEL: {
                auto& app = Application::Get();
                float x = event.wheel.x;
                float y = event.wheel.y;
                if (event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED) {
                    x *= -1;
                    y *= -1;
                }

                MouseScrolledEvent e(x, y);
                app.RaiseEvent(e);
                break;
            }
            default:
                break;
            }

            ImGui_ImplSDL3_ProcessEvent(&event);
        }
    }

    void SDLWindow::SetCursorMode(const CursorMode cursorMode) {
        switch (cursorMode) {
        case CursorMode::Normal:
            SDL_SetWindowRelativeMouseMode(m_SDLWindow, false);
            SDL_ShowCursor();
            break;
        case CursorMode::Locked:
            SDL_SetWindowRelativeMouseMode(m_SDLWindow, true);
            SDL_HideCursor();
            break;
        }
    }
}
