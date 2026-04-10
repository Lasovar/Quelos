#pragma once

#include "Quelos/Core/Base.h"
#include "Quelos/Core/GUID.h"

#include "flecs.h"
#include "ComponentReference.h"

namespace Quelos {
    using EntityID = GUID64;
    using RuntimeID = flecs::id_t;

    class QS_API Entity {
    public:
        Entity() = default;

        Entity(const flecs::entity id)
            : m_ID(id) {
        }

        bool operator==(const Entity& other) const {
            return m_ID == other.m_ID;
        }

        [[nodiscard]] bool IsAlive() const;
        [[nodiscard]] bool IsValid() const { return m_ID.is_valid(); }

        void Destruct() const;

        template <typename T>
        void Add() const {
            m_ID.add<T>();
        }

        void Add(const RuntimeID componentId) const {
            m_ID.add(componentId);
        }

        template <typename T>
        void Remove() const {
            m_ID.remove<T>();
        }

        void Remove(const RuntimeID componentId) const {
            m_ID.remove(componentId);
        }

        template <typename T>
        const flecs::entity& Set(T&& component) const {
            return m_ID.set<T>(component);
        }

        template <typename T, flecs::if_t<flecs::is_actual<T>::value> = 0>
        ComponentRef<T> GetRef() const {
            return m_ID.get_ref<T>();
        }

        template <typename T, typename A = flecs::actual_type_t<T>, flecs::if_t<flecs::is_pair<T>::value> = 0>
        [[nodiscard]] ComponentRef<A> GetRef() const {
            return m_ID.get_ref<T>();
        }

        template <typename T>
        [[nodiscard]] ComponentUntypedRef GetUntypedRef() const {
            return m_ID.get_ref<T>();
        }

        [[nodiscard]] ComponentUntypedRef GetUntypedRef(const RuntimeID id) const {
            return m_ID.get_ref(id);
        }

        template <typename First, typename Second, typename P = flecs::pair<First, Second>,
                  typename A = flecs::actual_type_t<P>>
        ComponentRef<A> GetRef() const {
            // TODO: check this too
            return m_ID.get_ref<First, Second>();
        }

        template <typename First>
        ComponentRef<First> GetRef(const Entity second) const {
            return m_ID.get_ref<First>(second.m_ID);
        }

        template <typename T>
        const T& Get() const {
            return m_ID.get<T>();
        }

        [[nodiscard]] const void* Get(const RuntimeID id) const {
            return m_ID.get(id);
        }

        template <typename T>
        const T* TryGet() const {
            return m_ID.try_get<T>();
        }

        template <typename T>
        T& GetMut() {
            return m_ID.get_mut<T>();
        }

        Entity GetParent() const {
            return m_ID.parent();
        }

        void SetParent(const Entity parent) const {
            m_ID.child_of(parent.GetInternalID());
        }

        [[nodiscard]] void* GetMut(const RuntimeID id) const {
            return m_ID.get_mut(id);
        }

        void SetName(const std::string& name) const {
            SetName(std::string_view(name));
        }

        void SetName(std::string_view name) const;

        [[nodiscard]] const char* GetName() const { return m_ID.name().c_str(); }

        [[nodiscard]] flecs::entity GetInternalID() const { return m_ID; }

        [[nodiscard]] int ChildrenCount() const { return m_ID.world().count(flecs::ChildOf, m_ID); }

        void Destroy() const {
            m_ID.destruct();
        }

        void RemoveParent() const {
            m_ID.remove(flecs::ChildOf, flecs::Wildcard);
        }

    protected:
        flecs::entity m_ID;
    };
}

namespace std {
    template <>
    struct hash<Quelos::Entity> {
        size_t operator()(const Quelos::Entity& e) const noexcept {
            return std::hash<ecs_id_t>{}(e.GetInternalID());
        }
    };
}
