#include <qspch.h>
#include "Mesh.h"

#include "Quelos/Renderer/Renderer.h"

namespace Quelos {
	Mesh::Mesh(std::vector<PosColorVertex> vertices, std::vector<uint16_t> indices) {
		m_Vertices = std::move(vertices);
		m_Indices = std::move(indices);

		m_VertexBuffer = Renderer::CreateVertexBuffer(m_Vertices);
		m_IndexBuffer = Renderer::CreateIndexBuffer(m_Indices);
	}
}
