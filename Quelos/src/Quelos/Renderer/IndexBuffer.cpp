#include <qspch.h>
#include "IndexBuffer.h"

namespace Quelos {

	IndexBuffer::IndexBuffer(uint16_t* indicies, uint32_t count) {
		m_Handle = bgfx::createIndexBuffer(bgfx::makeRef(indicies, count));
	}

	IndexBuffer::~IndexBuffer() {
		bgfx::destroy(m_Handle);
	}
}

