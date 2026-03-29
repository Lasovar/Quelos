#include "Mesh.h"

#include "Quelos/Renderer/Renderer.h"

namespace Quelos {
	Mesh::Mesh(const std::vector<PosColorVertex>& vertices, const std::vector<uint16_t>& indices) :
		m_Vertices(vertices),
		m_Indices(indices)
	{
		VertexLayout layout;
		layout.Add(VertexAttribute::Position, ShaderDataType::Float3);
		layout.Add(VertexAttribute::Color0, ShaderDataType::UNorm8x4);

		m_VertexBuffer = Renderer::CreateVertexBuffer(std::as_bytes(Span(m_Vertices)), layout);
		m_IndexBuffer = Renderer::CreateIndexBuffer(m_Indices);
	}
}
