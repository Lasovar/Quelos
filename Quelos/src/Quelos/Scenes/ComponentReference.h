#pragma once

#include "flecs/addons/cpp/ref.hpp"

namespace Quelos {
    template <typename Component>
    class CRef {
    public:
        CRef(flecs::ref<Component> ref) {
            m_Ref = ref;
        }

        Component* Get() { return m_Ref.get(); }
        Component* TryGet() { return m_Ref.try_get(); }

        Component* operator->() { return m_Ref.operator->(); }
    private:
        flecs::ref<Component> m_Ref{};
    };

    class CUntypedRef {
    public:
        CUntypedRef(const flecs::untyped_ref& ref) {
            m_Ref = ref;
        }

        void* Get() { return m_Ref.get(); }
        void* TryGet() { return m_Ref.try_get(); }
    private:
        flecs::untyped_ref m_Ref{};
    };
}
