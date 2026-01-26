#pragma once

#include <bgfx/bgfx.h>

namespace Quelos {
	class IndexBuffer : public RefCounted {
	public:
		explicit IndexBuffer(const std::vector<uint16_t>& indices);
		~IndexBuffer() override;

		bgfx::IndexBufferHandle GetHandle() const { return m_Handle; }

	private:
		bgfx::IndexBufferHandle m_Handle{};
	};
}

