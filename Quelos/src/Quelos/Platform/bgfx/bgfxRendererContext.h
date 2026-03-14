#pragma once
#include "Quelos/Renderer/RendererContext.h"

namespace Quelos {
    class bgfxRendererContext : public RendererContext {
    public:
        void Init(const Ref<Window>& window, RendererAPI api) override;

        VertexBufferHandle CreateVertexBuffer(const std::vector<PosColorVertex>& vertices) override;
        void BindVertexBuffer(VertexBufferHandle vertexBufferHandle, uint32_t stream) override;
        void Destroy(VertexBufferHandle vertexBufferHandle) override;

        IndexBufferHandle CreateIndexBuffer(const std::vector<uint16_t>& indices) override;
        void BindIndexBuffer(IndexBufferHandle indexBufferHandle) override;
        void Destroy(IndexBufferHandle indexBufferHandle) override;
    };
}
