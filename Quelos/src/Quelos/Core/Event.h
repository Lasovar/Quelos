#pragma once

#include <functional>

#include "Base.h"

namespace Quelos {
    enum class EventType {
        None = 0,
        WindowClose, WindowResize,
        KeyPressed, KeyReleased,
        MouseButtonPressed, MouseButtonReleased, MouseMoved, MouseScrolled,
    };

    enum EventCategory
    {
        None = 0,
        EventCategoryApplication		= GetBit(0),
        EventCategoryInput				= GetBit(1),
        EventCategoryKeyboard			= GetBit(2),
        EventCategoryMouse				= GetBit(3),
        EventCategoryMouseButton		= GetBit(4)
    };

#define EVENT_CLASS_TYPE(type) static EventType GetStaticType() { return EventType::type; }\
virtual EventType GetEventType() const override { return GetStaticType(); }\
virtual const char* GetName() const override { return #type; }

#define EVENT_CLASS_CATEGORY(category) virtual int GetCategoryFlags() const override {return category;}

    class Event {
    public:
        bool Handled = false;

        virtual ~Event() {
        }

        virtual EventType GetEventType() const = 0;
        virtual int GetCategoryFlags() const = 0;
        virtual const char* GetName() const = 0;
        virtual std::string ToString() const { return GetName(); }


        bool IsInCategory(const EventCategory category) const {
            return GetCategoryFlags() & category;
        }
    };

    class EventDispatcher {
        template <typename T>
        using EventFn = std::function<bool(T&)>;

    public:
        EventDispatcher(Event& event)
            : m_Event(event) {
        }

        template <typename T>
        bool Dispatch(EventFn<T> func) {
            if (m_Event.GetEventType() == T::GetStaticType() && !m_Event.Handled) {
                m_Event.Handled = func(*static_cast<T*>(&m_Event));
                return true;
            }

            return false;
        }

    private:
        Event& m_Event;
    };
}
