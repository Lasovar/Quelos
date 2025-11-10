#include "qspch.h"
#include "VertexBuffer.h"

namespace Quelos {

	VertexBuffer::VertexBuffer(glm::vec3* verticies, uint32_t count) {
		using namespace bgfx;

		VertexLayout vertexBufferLayout;
		vertexBufferLayout
			.begin()
			.add(Attrib::Position, 3, AttribType::Float)
			.end();

		m_Handle = createVertexBuffer(makeRef(verticies, count), vertexBufferLayout);
	}

	VertexBuffer::~VertexBuffer() {
		bgfx::destroy(m_Handle);
	}

}

