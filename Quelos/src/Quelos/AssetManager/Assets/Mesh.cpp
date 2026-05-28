#include "Mesh.h"

#include "Quelos/Renderer/Renderer.h"
#include "Quelos/Renderer/VertexBufferLayout.h"

namespace Quelos {
    Mesh::Mesh(AssetRef<Model> model, MeshData* meshData) : m_MeshData(meshData), m_Model(std::move(model)) {
        if (!m_MeshData) {
            QS_CORE_ERROR_TAG("Mesh", "Failed to created mesh! invalid mesh data");
            return;
        }

        VertexLayout layout;
        layout.Add(VertexAttribute::Position, ValueType::Float3);
        layout.Add(VertexAttribute::Normal, ValueType::Float3);
        layout.Add(VertexAttribute::Tangent, ValueType::Float3);
        layout.Add(VertexAttribute::TexCoord0, ValueType::Float2);

        m_VertexBuffer = Renderer::CreateVertexBuffer(std::as_bytes(Span(meshData->Vertices)), layout);
        m_IndexBuffer = Renderer::CreateIndexBuffer(meshData->Indices);

        SetAssetID(meshData->AssetId);
    }

    Mesh::~Mesh() {
        Renderer::Destroy(m_VertexBuffer);
        Renderer::Destroy(m_IndexBuffer);
    }
}
