#pragma once

#include <Quelos/Renderer/VertexBuffer.h>
#include <Quelos/Renderer/IndexBuffer.h>

#include "Quelos/AssetManager/Asset.h"
#include "Quelos/Serialization/Serializer.h"

namespace Quelos {
	class Model;

	class QS_API Mesh : public Asset {
	public:
		Mesh(Vec<Vertex> vertices, Vec<uint16_t> indices, const Ref<Model>& model, std::string name = "");

		Vec<Vertex>& GetVertices() { return m_Vertices; }
		Vec<uint16_t>& GetIndices() { return m_Indices; }

		[[nodiscard]] VertexBufferHandle GetVertexBuffer() const { return m_VertexBuffer; }
		[[nodiscard]] IndexBufferHandle GetIndexBuffer() const { return m_IndexBuffer; }

		const AssetType& GetAssetType() const override { return GetStaticType(); }
		const std::string& GetName() { return m_Name; }
		static const AssetType& GetStaticType() {
			static AssetType assetType = Quelos::GetAssetType<Mesh>();
			return assetType;
		}

	private:
		VertexBufferHandle m_VertexBuffer;
		IndexBufferHandle m_IndexBuffer;

		Ref<Model> m_Model;
		std::string m_Name;

		Vec<Vertex> m_Vertices;
		Vec<uint16_t> m_Indices;
	};
}
