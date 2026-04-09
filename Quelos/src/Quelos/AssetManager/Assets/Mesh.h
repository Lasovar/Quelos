#pragma once

#include <Quelos/Renderer/VertexBuffer.h>
#include <Quelos/Renderer/IndexBuffer.h>

#include "Quelos/AssetManager/Asset.h"

namespace Quelos {
	class QS_API Mesh : public Asset {
	public:
		Mesh(const std::vector<PosColorVertex>& vertices, const std::vector<uint16_t>& indices);

		std::vector<PosColorVertex>& GetVertices() { return m_Vertices; }
		std::vector<uint16_t>& GetIndices() { return m_Indices; }

		[[nodiscard]] VertexBufferHandle GetVertexBuffer() const { return m_VertexBuffer; }
		[[nodiscard]] IndexBufferHandle GetIndexBuffer() const { return m_IndexBuffer; }

		AssetType GetAssetType() const override { return GetStaticType(); }
		static AssetType GetStaticType() { return AssetType::Mesh; }

	private:
		VertexBufferHandle m_VertexBuffer;
		IndexBufferHandle m_IndexBuffer;

		std::vector<PosColorVertex> m_Vertices;
		std::vector<uint16_t> m_Indices;
	};
}
