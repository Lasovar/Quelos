#pragma once

#include <bgfx/bgfx.h>

namespace Quelos {
	class IndexBuffer : public RefCounted {
	public:
		IndexBuffer(uint16_t* indicies, uint32_t count);
		~IndexBuffer();

		bgfx::IndexBufferHandle GetHandle() const { return m_Handle; }

	private:
		bgfx::IndexBufferHandle m_Handle;
	};
}

