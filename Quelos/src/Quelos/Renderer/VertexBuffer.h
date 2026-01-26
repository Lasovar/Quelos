#pragma once

#include <bgfx/bgfx.h>
#include <glm/glm.hpp>

namespace Quelos {
	struct PosColorVertex {
		float x;
		float y;
		float z;
		uint32_t abgr;
	};


	class VertexBuffer : public RefCounted {
	public:
		explicit VertexBuffer(const std::vector<PosColorVertex>& vertices);
		~VertexBuffer() override;

		bgfx::VertexBufferHandle GetHandle() const { return m_Handle; }

	private:
		bgfx::VertexBufferHandle m_Handle{};
	};
}

