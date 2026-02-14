#pragma once

#include <format>
#include <string_view>

#include "Quelos/Core/Event.h"

namespace Quelos {
    enum class KeyCode : uint16_t {
        Space = 32,
        Apostrophe = 39, /* ' */
        Comma = 44, /* , */
        Minus = 45, /* - */
        Period = 46, /* . */
        Slash = 47, /* / */
        Alpha0 = 48,
        Alpha1 = 49,
        Alpha2 = 50,
        Alpha3 = 51,
        Alpha4 = 52,
        Alpha5 = 53,
        Alpha6 = 54,
        Alpha7 = 55,
        Alpha8 = 56,
        Alpha9 = 57,
        Semicolon = 59, /* ; */
        Equal = 61, /* = */
        A = 65,
        B = 66,
        C = 67,
        D = 68,
        E = 69,
        F = 70,
        G = 71,
        H = 72,
        I = 73,
        J = 74,
        K = 75,
        L = 76,
        M = 77,
        N = 78,
        O = 79,
        P = 80,
        Q = 81,
        R = 82,
        S = 83,
        T = 84,
        U = 85,
        V = 86,
        W = 87,
        X = 88,
        Y = 89,
        Z = 90,
        LeftBracket = 91, /* [ */
        Backslash = 92, /* \ */
        RightBracket = 93, /* ] */
        GraveAccent = 96, /* ` */
        World_1 = 161,
        World_2 = 162,

        Escape = 256,
        Enter = 257,
        Tab = 258,
        Backspace = 259,
        Insert = 260,
        Delete = 261,
        Right = 262,
        Left = 263,
        Down = 264,
        Up = 265,
        Page_up = 266,
        Page_down = 267,
        Home = 268,
        End = 269,
        Caps_lock = 280,
        Scroll_lock = 281,
        Num_lock = 282,
        Print_screen = 283,
        Pause = 284,
        F1 = 290,
        F2 = 291,
        F3 = 292,
        F4 = 293,
        F5 = 294,
        F6 = 295,
        F7 = 296,
        F8 = 297,
        F9 = 298,
        F10 = 299,
        F11 = 300,
        F12 = 301,
        F13 = 302,
        F14 = 303,
        F15 = 304,
        F16 = 305,
        F17 = 306,
        F18 = 307,
        F19 = 308,
        F20 = 309,
        F21 = 310,
        F22 = 311,
        F23 = 312,
        F24 = 313,
        F25 = 314,
        Kp0 = 320,
        Kp1 = 321,
        Kp2 = 322,
        Kp3 = 323,
        Kp4 = 324,
        Kp5 = 325,
        Kp6 = 326,
        Kp7 = 327,
        Kp8 = 328,
        Kp9 = 329,
        KpDecimal = 330,
        KpDivide = 331,
        KpMultiply = 332,
        KpSubtract = 333,
        KpAdd = 334,
        KpEnter = 335,
        KpEqual = 336,
        LeftShift = 340,
        LeftControl = 341,
        LeftAlt = 342,
        LeftSuper = 343,
        RightShift = 344,
        RightControl = 345,
        RightAlt = 346,
        RightSuper = 347,
        Menu = 348,

        Last = Menu
    };

    constexpr std::string_view KeyCodeToString(const KeyCode key) {
        switch (key) {
        case KeyCode::Space: return "Space";
        case KeyCode::Apostrophe: return "Apostrophe";
        case KeyCode::Comma: return "Comma";
        case KeyCode::Minus: return "Minus";
        case KeyCode::Period: return "Period";
        case KeyCode::Slash: return "Slash";

        case KeyCode::Alpha0: return "0";
        case KeyCode::Alpha1: return "1";
        case KeyCode::Alpha2: return "2";
        case KeyCode::Alpha3: return "3";
        case KeyCode::Alpha4: return "4";
        case KeyCode::Alpha5: return "5";
        case KeyCode::Alpha6: return "6";
        case KeyCode::Alpha7: return "7";
        case KeyCode::Alpha8: return "8";
        case KeyCode::Alpha9: return "9";

        case KeyCode::Semicolon: return "Semicolon";
        case KeyCode::Equal: return "Equal";

        case KeyCode::A: return "A";
        case KeyCode::B: return "B";
        case KeyCode::C: return "C";
        case KeyCode::D: return "D";
        case KeyCode::E: return "E";
        case KeyCode::F: return "F";
        case KeyCode::G: return "G";
        case KeyCode::H: return "H";
        case KeyCode::I: return "I";
        case KeyCode::J: return "J";
        case KeyCode::K: return "K";
        case KeyCode::L: return "L";
        case KeyCode::M: return "M";
        case KeyCode::N: return "N";
        case KeyCode::O: return "O";
        case KeyCode::P: return "P";
        case KeyCode::Q: return "Q";
        case KeyCode::R: return "R";
        case KeyCode::S: return "S";
        case KeyCode::T: return "T";
        case KeyCode::U: return "U";
        case KeyCode::V: return "V";
        case KeyCode::W: return "W";
        case KeyCode::X: return "X";
        case KeyCode::Y: return "Y";
        case KeyCode::Z: return "Z";

        case KeyCode::Escape: return "Escape";
        case KeyCode::Enter: return "Enter";
        case KeyCode::Tab: return "Tab";
        case KeyCode::Backspace: return "Backspace";
        case KeyCode::Insert: return "Insert";
        case KeyCode::Delete: return "Delete";

        case KeyCode::Right: return "Right";
        case KeyCode::Left: return "Left";
        case KeyCode::Down: return "Down";
        case KeyCode::Up: return "Up";

        case KeyCode::Page_up: return "PageUp";
        case KeyCode::Page_down: return "PageDown";
        case KeyCode::Home: return "Home";
        case KeyCode::End: return "End";

        case KeyCode::Caps_lock: return "CapsLock";
        case KeyCode::Scroll_lock: return "ScrollLock";
        case KeyCode::Num_lock: return "NumLock";
        case KeyCode::Print_screen: return "PrintScreen";
        case KeyCode::Pause: return "Pause";

        case KeyCode::F1: return "F1";
        case KeyCode::F2: return "F2";
        case KeyCode::F3: return "F3";
        case KeyCode::F4: return "F4";
        case KeyCode::F5: return "F5";
        case KeyCode::F6: return "F6";
        case KeyCode::F7: return "F7";
        case KeyCode::F8: return "F8";
        case KeyCode::F9: return "F9";
        case KeyCode::F10: return "F10";
        case KeyCode::F11: return "F11";
        case KeyCode::F12: return "F12";

        case KeyCode::LeftShift: return "LeftShift";
        case KeyCode::LeftControl: return "LeftControl";
        case KeyCode::LeftAlt: return "LeftAlt";
        case KeyCode::LeftSuper: return "LeftSuper";
        case KeyCode::RightShift: return "RightShift";
        case KeyCode::RightControl: return "RightControl";
        case KeyCode::RightAlt: return "RightAlt";
        case KeyCode::RightSuper: return "RightSuper";
        case KeyCode::Menu: return "Menu";

        default: return "Unknown";
        }
    }

    class KeyEvent : public Event {
    public:
        KeyCode GetKeyCode() const { return m_KeyCode; }

        EVENT_CLASS_CATEGORY(EventCategoryKeyboard | EventCategoryInput)

    protected:
        explicit KeyEvent(const KeyCode keycode)
            : m_KeyCode(keycode) {
        }

        KeyCode m_KeyCode;
    };

    class KeyPressedEvent : public KeyEvent {
    public:
        KeyPressedEvent(const KeyCode keycode, const bool isRepeat)
            : KeyEvent(keycode), m_IsRepeat(isRepeat) {
        }

        bool IsRepeat() const { return m_IsRepeat; }

        std::string ToString() const override {
            return std::format("KeyPressed: {} (Repeat?: )", KeyCodeToString(m_KeyCode), m_IsRepeat);
        }

        EVENT_CLASS_TYPE(KeyPressed)

    private:
        bool m_IsRepeat;
    };

    class KeyReleasedEvent : public KeyEvent {
    public:
        explicit KeyReleasedEvent(const KeyCode keycode)
            : KeyEvent(keycode) {
        }

        std::string ToString() const override {
            return std::format("KeyReleased: {}", KeyCodeToString(m_KeyCode));
        }

        EVENT_CLASS_TYPE(KeyReleased)
    };

    /*
    class KeyTypedEvent : public KeyEvent {
    public:
        explicit KeyTypedEvent(const int keycode)
            : KeyEvent(keycode) {}

        std::string ToString() const override {
            return std::format("KeyTyped: {}", m_KeyCode);
        }

        EVENT_CLASS_TYPE(KeyTyped)
    };
    */

    enum class MouseButton {
        Left = 0,
        Right = 1,
        Middle = 2,
        X1 = 3,
        X2 = 4,

        Last = X2,
    };

    constexpr std::string_view MouseButtonToString(const MouseButton button)
    {
        switch (button)
        {
        case MouseButton::Left:   return "Left";
        case MouseButton::Middle: return "Middle";
        case MouseButton::Right:  return "Right";
        case MouseButton::X1:     return "X1";
        case MouseButton::X2:     return "X2";
        default:                      return "Unknown";
        }
    }

    class MouseButtonEvent : public Event {
    public:
        MouseButton GetMouseButton() const { return m_Button; }

        EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryMouseButton | EventCategoryInput)

    protected:
        explicit MouseButtonEvent(const MouseButton button)
            : m_Button(button) {
        }

        MouseButton m_Button;
    };

    class MouseButtonPressedEvent : public MouseButtonEvent {
    public:
        explicit MouseButtonPressedEvent(const MouseButton button)
            : MouseButtonEvent(button) {
        }

        std::string ToString() const override {
            return std::format("MouseButtonPressed: {}", MouseButtonToString(m_Button));
        }

        EVENT_CLASS_TYPE(MouseButtonPressed)
    };

    class MouseButtonReleasedEvent : public MouseButtonEvent {
    public:
        explicit MouseButtonReleasedEvent(const MouseButton button)
            : MouseButtonEvent(button) {
        }

        std::string ToString() const override {
            return std::format("MouseButtonReleased: {}", MouseButtonToString(m_Button));
        }

        EVENT_CLASS_TYPE(MouseButtonReleased)
    };

    class MouseMovedEvent : public Event {
    public:
        MouseMovedEvent(const float x, const float y, const float deltax, float deltay)
        : m_Position(x, y), m_Delta(deltax, deltay) {
        }

        float GetX() const { return m_Position.x; }
        float GetY() const { return m_Position.y; }

        glm::vec2 GetPosition() const { return m_Position; }

        float GetDeltaX() const { return m_Delta.x; };
        float GetDeltaY() const { return m_Delta.y; }

        glm::vec2 GetDelta() const { return m_Delta; }

        std::string ToString() const override {
            return std::format("MouseMoved: ({}, {})", m_Position.x, m_Position.y);
        }

        EVENT_CLASS_TYPE(MouseMoved)
        EVENT_CLASS_CATEGORY(EventCategoryInput | EventCategoryMouse)

    private:
        glm::vec2 m_Position;
        glm::vec2 m_Delta;
    };

    class MouseScrolledEvent : public Event {
    public:
        MouseScrolledEvent(const float x, const float y) : xOffset(x), yOffset(y) {
        }

        float GetXOffset() const { return xOffset; }
        float GetYOffset() const { return yOffset; }

        std::string ToString() const override {
            return
                std::format("MouseScrolled: ({}, {})", xOffset, yOffset);
        }

        EVENT_CLASS_TYPE(MouseScrolled)
        EVENT_CLASS_CATEGORY(EventCategoryInput | EventCategoryMouse)

    private:
        float xOffset;
        float yOffset;
    };
}
