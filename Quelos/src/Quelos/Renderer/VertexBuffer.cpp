#include "qspch.h"
#include "VertexBuffer.h"

namespace Quelos {
	VertexBuffer::VertexBuffer(const std::vector<PosColorVertex>& vertices) {
		bgfx::VertexLayout pvcLayout;
		pvcLayout
			.begin()
			.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
			.add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
			.end();

		m_Handle = createVertexBuffer(bgfx::makeRef(vertices.data(), vertices.size() * sizeof(PosColorVertex)), pvcLayout);

		if (!bgfx::isValid(m_Handle)) {
			QS_CORE_ERROR("Failed to create VertexBuffer!");
		}
	}

	VertexBuffer::~VertexBuffer() {
		bgfx::destroy(m_Handle);
	}
}

