#pragma once

#include "flecs.h"

namespace Quelos {
    using RuntimeID = ecs_id_t;
    template <typename TComponent>
    class ComponentRef {
    public:
        ComponentRef(flecs::ref<TComponent> ref) {
            m_Ref = ref;
        }

        TComponent* Get() { return m_Ref.get(); }
        TComponent* TryGet() { return m_Ref.try_get(); }

        TComponent* operator->() { return m_Ref.operator->(); }
    private:
        flecs::ref<TComponent> m_Ref{};
    };

    class ComponentUntypedRef {
    public:
        ComponentUntypedRef() = default;

        ComponentUntypedRef(const flecs::untyped_ref& ref) {
            m_Ref = ref;
        }

        void* Get() { return m_Ref.get(); }
        void* TryGet() { return m_Ref.try_get(); }

        [[nodiscard]] flecs::entity GetEntityID() const { return m_Ref.entity(); }
        [[nodiscard]] RuntimeID GetID() const { return m_Ref.component(); }
    private:
        flecs::untyped_ref m_Ref{};
    };
}
