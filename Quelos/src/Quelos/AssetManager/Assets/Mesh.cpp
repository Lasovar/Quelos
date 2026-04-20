#include "Mesh.h"

#include "Quelos/AssetManager/Assets/Model.h"
#include "Quelos/Renderer/Renderer.h"

namespace Quelos {
	Mesh::Mesh(Vec<Vertex> vertices, Vec<uint16_t> indices, const Ref<Model>& model, std::string name) :
		m_Model(model),
		m_Name(std::move(name)),
		m_Vertices(std::move(vertices)),
		m_Indices(std::move(indices))
	{
		VertexLayout layout;
		layout.Add(VertexAttribute::Position, ShaderDataType::Float3);
		layout.Add(VertexAttribute::Normal, ShaderDataType::Float3);
		layout.Add(VertexAttribute::Tangent, ShaderDataType::Float3);
		layout.Add(VertexAttribute::TexCoord0, ShaderDataType::Float2);

		m_VertexBuffer = Renderer::CreateVertexBuffer(std::as_bytes(Span(m_Vertices)), layout);
		m_IndexBuffer = Renderer::CreateIndexBuffer(m_Indices);
	}
}
