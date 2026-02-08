#pragma once

#include <Quelos/Renderer/VertexBuffer.h>
#include <Quelos/Renderer/IndexBuffer.h>

#include "Quelos/AssetManager/Asset.h"

namespace Quelos {
	class Mesh : public AssetHandle {
	public:
		Mesh(const std::vector<PosColorVertex>& vertices, const std::vector<uint16_t>& indices);

		Ref<VertexBuffer> GetVertexBuffer() const { return m_VertexBuffer; }
		Ref<IndexBuffer> GetIndexBuffer() const { return m_IndexBuffer; }
	private:
		Ref<VertexBuffer> m_VertexBuffer;
		Ref<IndexBuffer> m_IndexBuffer;
	};
}
