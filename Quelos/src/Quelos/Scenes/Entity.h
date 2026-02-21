#pragma once

#include "Quelos/Core/GUID.h"

#include "flecs.h"
#include "ComponentReference.h"

namespace Quelos {
	using EntityID = GUID64;

	class Entity {
	public:
		Entity() = default;
		Entity(const flecs::entity id)
			: m_ID(id) { }

		[[nodiscard]] bool IsAlive() const;

		void Destruct();

		template <typename T>
		const flecs::entity& Add() const {
			return m_ID.add<T>();
		}

		template <typename T>
		const flecs::entity& Set(T&& component) const {
			return m_ID.set<T>(component);
		}

		template <typename T, flecs::if_t<flecs::is_actual<T>::value > = 0>
		CRef<T> GetRef() const {
			return m_ID.get_ref<T>();
		}

		template <typename T, typename A = flecs::actual_type_t<T>, flecs::if_t< flecs::is_pair<T>::value > = 0>
		CRef<A> GetRef() const {
			// TODO: check wtf this is
			return m_ID.get_ref<T>();
		}

		template <typename First, typename Second, typename P = flecs::pair<First, Second>,
			typename A = flecs::actual_type_t<P>>
		CRef<A> GetRef() const {
			// TODO: check this too
			return m_ID.get_ref<First, Second>();
		}

		template <typename First>
		CRef<First> GetRef(const Entity second) const {
			return m_ID.get_ref<First>(second.m_ID);
		}

		template <typename T>
		const T& Get() const {
			return m_ID.get<T>();
		}

		void SetName(const std::string& name) const { m_ID.set_name(name.c_str()); }
		[[nodiscard]] std::string_view GetName() const { return m_ID.name().c_str(); }

		[[nodiscard]] flecs::entity GetID() const { return m_ID; }
	private:
		flecs::entity m_ID;
	};
}
