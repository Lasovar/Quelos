#pragma once

#include <cstdint>
#include <format>
#include <string>

#include "Quelos/Core/Event.h"

namespace Quelos {
    class WindowClosedEvent : public Event {
    public:
        WindowClosedEvent() = default;

        EVENT_CLASS_TYPE(WindowClose)
		EVENT_CLASS_CATEGORY(EventCategoryApplication)
    };

    class WindowResizeEvent : public Event
    {
    public:
        WindowResizeEvent(const uint32_t width, const uint32_t height)
            : m_Width(width), m_Height(height) { }

        inline uint32_t GetWidth() const { return m_Width; }
        inline uint32_t GetHeight() const { return m_Height; }

        [[nodiscard]]
        std::string ToString() const override {
            return std::format("WindowResizeEvent: {}, {}", m_Width, m_Height);
        }

        EVENT_CLASS_TYPE(WindowResize)
		EVENT_CLASS_CATEGORY(EventCategoryApplication)
    private:
        uint32_t m_Width, m_Height;
    };
}
