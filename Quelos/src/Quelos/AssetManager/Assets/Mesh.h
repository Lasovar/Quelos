#pragma once

#include <Quelos/Renderer/VertexBuffer.h>
#include <Quelos/Renderer/IndexBuffer.h>

#include "Quelos/AssetManager/Asset.h"

namespace Quelos {
	class Mesh : public AssetHandle {
	public:
		Mesh(std::vector<PosColorVertex> vertices, std::vector<uint16_t> indices);

		std::vector<PosColorVertex>& GetVertices() { return m_Vertices; }
		std::vector<uint16_t>& GetIndices() { return m_Indices; }

		[[nodiscard]] VertexBufferHandle GetVertexBuffer() const { return m_VertexBuffer; }
		[[nodiscard]] IndexBufferHandle GetIndexBuffer() const { return m_IndexBuffer; }
	private:
		VertexBufferHandle m_VertexBuffer;
		IndexBufferHandle m_IndexBuffer;

		std::vector<PosColorVertex> m_Vertices;
		std::vector<uint16_t> m_Indices;
	};
}
