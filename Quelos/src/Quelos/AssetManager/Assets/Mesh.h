#pragma once

#include <Quelos/Renderer/VertexBuffer.h>
#include <Quelos/Renderer/IndexBuffer.h>

#include "Quelos/AssetManager/Asset.h"
#include "Quelos/Renderer/MeshData.h"
#include "Quelos/Renderer/ShaderResourceBinding.h"
#include "Quelos/Serialization/Serializer.h"

#include "Model.h"

namespace Quelos {

	class QS_API Mesh : public Asset {
	public:
		explicit Mesh(AssetRef<Model> model, MeshData* meshData);
		~Mesh() override;

		Vec<Vertex>& GetVertices() const { return m_MeshData->Vertices; }
		Vec<uint16_t>& GetIndices() const { return m_MeshData->Indices; }

		[[nodiscard]] VertexBufferHandle GetVertexBuffer() const { return m_VertexBuffer; }
		[[nodiscard]] IndexBufferHandle GetIndexBuffer() const { return m_IndexBuffer; }

		const AssetType& GetAssetType() const override { return GetStaticType(); }
		const std::string& GetName() const { return m_MeshData->Name; }

		static const AssetType& GetStaticType() {
			static AssetType assetType = Quelos::GetAssetType<Mesh>();
			return assetType;
		}

	private:
		VertexBufferHandle m_VertexBuffer;
		IndexBufferHandle m_IndexBuffer;

		MeshData* m_MeshData;
		AssetRef<Model> m_Model;
	};
}
