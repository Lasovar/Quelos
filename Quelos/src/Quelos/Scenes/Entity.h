#pragma once

#include "flecs.h"

namespace Quelos {
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

		template <typename T>
		const T& Get() const {
			return m_ID.get<T>();
		}

	private:
		flecs::entity m_ID;
	};
}
