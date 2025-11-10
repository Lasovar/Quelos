#pragma once

#include "flecs.h"

namespace Quelos {
	class Entity {
	public:
		Entity() {}
		Entity(flecs::entity id)
			: m_ID(id) { }

		bool IsAlive() const;

		void Destruct();

		template <typename T>
		const flecs::entity& Add() const {
			return m_ID.add<T>();
		}

		template <typename T>
		const T& Get() const {
			return m_ID.get<T>();
		}

	private:
		flecs::entity m_ID;
	};
}

