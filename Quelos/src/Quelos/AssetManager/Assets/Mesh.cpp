#include "Mesh.h"

#include "Quelos/Renderer/Renderer.h"
#include "Quelos/Renderer/VertexBufferLayout.h"

namespace Quelos {
    Mesh::Mesh(MeshData* meshData) : m_MeshData(meshData) {
        if (!m_MeshData) {
            QS_CORE_ERROR_TAG("Mesh", "Failed to created mesh! invalid mesh data");
            return;
        }

        VertexLayout layout;
        layout.Add(VertexAttribute::Position, ShaderDataType::Float3);
        layout.Add(VertexAttribute::Normal, ShaderDataType::Float3);
        layout.Add(VertexAttribute::Tangent, ShaderDataType::Float3);
        layout.Add(VertexAttribute::TexCoord0, ShaderDataType::Float2);

        m_VertexBuffer = Renderer::CreateVertexBuffer(std::as_bytes(Span(meshData->Vertices)), layout);
        m_IndexBuffer = Renderer::CreateIndexBuffer(meshData->Indices);

        SetAssetID(meshData->AssetId);
    }

    Mesh::~Mesh() {
        Renderer::Destroy(m_VertexBuffer);
        Renderer::Destroy(m_IndexBuffer);
    }
}
