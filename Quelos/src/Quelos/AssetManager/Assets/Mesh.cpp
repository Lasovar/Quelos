#include "Mesh.h"

#include "Quelos/Renderer/Renderer.h"

namespace Quelos {
	Mesh::Mesh(const std::vector<PosColorVertex>& vertices, const std::vector<uint16_t>& indices) :
		m_Vertices(vertices),
		m_Indices(indices)
	{
		m_VertexBuffer = Renderer::CreateVertexBuffer(m_Vertices);
		m_IndexBuffer = Renderer::CreateIndexBuffer(m_Indices);
	}
}
