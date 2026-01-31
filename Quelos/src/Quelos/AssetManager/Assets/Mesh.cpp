#include <qspch.h>
#include "Mesh.h"

namespace Quelos {
	Mesh::Mesh(const std::vector<PosColorVertex>& vertices, const std::vector<uint16_t>& indices) {
		m_VertexBuffer = VertexBuffer::Create(vertices);
		m_IndexBuffer = IndexBuffer::Create(indices);
	}

}
