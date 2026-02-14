#pragma once

#include "Entity.h"
#include <Quelos/Core/Ref.h>

#include "Quelos/Core/Event.h"
#include "Quelos/Renderer/FrameBuffer.h"

namespace Quelos {
	enum class SystemGroup : uint8_t {
		OnStart,
		OnLoad,
		PostLoad,
		OnUpdate,
		PreUpdate,
		OnValidate,
		PostUpdate,
		PreStore,
		OnStore
	};

	constexpr flecs::entity_t SystemGroupToFlecsKind(const SystemGroup systemGroup) {
		switch (systemGroup) {
		case SystemGroup::OnStart: return flecs::OnStart;
		case SystemGroup::OnLoad: return flecs::OnLoad;
		case SystemGroup::PostLoad: return flecs::PostLoad;
		case SystemGroup::OnUpdate: return flecs::OnUpdate;
		case SystemGroup::PreUpdate: return flecs::PreUpdate;
		case SystemGroup::OnValidate: return flecs::OnValidate;
		case SystemGroup::PostUpdate: return flecs::PostUpdate;
		case SystemGroup::PreStore: return flecs::PreStore;
		case SystemGroup::OnStore: return flecs::OnStore;
		default: return flecs::OnUpdate;
		}
	}

	class Scene : public RefCounted {
	public:
		explicit Scene(std::string name = "Untitled Scene");

		template <typename Func>
		void Each(Func&& func) const {
			flecs::_::query_delegate<Func> f_delegate(m_World, FLECS_MOV(func));
		}

		template <typename... Comps, typename... Args, typename Func>
		void System(const SystemGroup systemGroup, Func&& func, Args&&... args) const {
			m_World.system<Comps...>(std::forward<Args>(args)...)
				.kind(SystemGroupToFlecsKind(systemGroup))
				.each(func);
		}

		template <typename... Comps, typename... Args, typename Func>
		void System(Func&& func, Args&&... args) const {
			System<Comps...>(
				SystemGroup::OnUpdate,
				std::forward<Func>(func),
				std::forward<Args>(args)...
			);
		}

		void Tick(float deltaTime) const;
		void StartRender(const Ref<FrameBuffer>& frameBuffer) const;
		void Render(uint32_t viewId) const;
		void EndRender() const;

		const std::string& GetName() const { return m_Name; }

		Entity CreateEntity(const std::string& entityName);
		Entity CreateEntity(const EntityID& guid, const std::string& entityName);
		void OnViewportResized(glm::vec2 viewportSize) const;

		flecs::world GetWorld() const { return m_World; }
	private:
		std::unordered_map<EntityID, Entity> m_EntityMap;

		flecs::world m_World;
		std::string m_Name;
	};
}

