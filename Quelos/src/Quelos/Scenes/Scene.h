#pragma once

#include "Entity.h"
#include <Quelos/Core/Ref.h>

#include "Quelos/Renderer/FrameBuffer.h"

namespace Quelos {
	class Scene : public RefCounted {
	public:
		explicit Scene(std::string name = "Untitled Scene");

		void Tick(float deltaTime) const;
		void Render(uint32_t viewId, const Ref<FrameBuffer>& frameBuffer) const;

		const std::string& GetName() const { return m_Name; }

		Entity CreateEntity(const std::string& entityName) const;
		flecs::world GetWorld() const { return m_World; }
	private:
		flecs::world m_World;
		std::string m_Name;
	};
}

