#include <qspch.h>
#include "Mesh.h"

namespace Quelos {
	Mesh::Mesh(std::vector<PosColorVertex> vertices, std::vector<uint16_t> indices) {
		m_Vertices = std::move(vertices);
		m_Indices = std::move(indices);

		m_VertexBuffer = VertexBuffer::Create(m_Vertices);
		m_IndexBuffer = IndexBuffer::Create(m_Indices);
	}
}
