#pragma once

#include <bgfx/bgfx.h>
#include <glm/glm.hpp>

namespace Quelos {
	class VertexBuffer : public RefCounted {
	public:
		VertexBuffer(glm::vec3* verticies, uint32_t count);
		~VertexBuffer();

		bgfx::VertexBufferHandle GetHandle() const { return m_Handle; }

	private:
		bgfx::VertexBufferHandle m_Handle;
	};
}

