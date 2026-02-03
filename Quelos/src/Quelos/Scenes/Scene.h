#pragma once

#include "Entity.h"
#include <Quelos/Core/Ref.h>

#include "Quelos/Renderer/FrameBuffer.h"

namespace Quelos {
	class Scene : public RefCounted {
	public:
		Scene();

		void Tick(float deltaTime) const;
		void Render(uint32_t viewId, Ref<FrameBuffer> frameBuffer) const;

		Entity CreateEntity(const std::string& entityName) const;
		flecs::world GetWorld() const { return m_World; }
	private:
		flecs::world m_World;
	};
}

