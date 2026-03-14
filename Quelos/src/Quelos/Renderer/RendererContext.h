#pragma once

#include "IndexBuffer.h"
#include "RendererAPI.h"
#include "VertexBuffer.h"
#include "Quelos/Core/Window.h"

namespace Quelos {
    class RendererContext {
    public:
        virtual void Init(const Ref<Window>& ref, RendererAPI api) = 0;

        virtual ~RendererContext() = default;

        virtual VertexBufferHandle CreateVertexBuffer(const std::vector<PosColorVertex>& vertices) = 0;
        virtual void BindVertexBuffer(VertexBufferHandle vertexBufferHandle, uint32_t stream) = 0;

        virtual IndexBufferHandle CreateIndexBuffer(const std::vector<uint16_t>& vertices) = 0;
        virtual void BindIndexBuffer(IndexBufferHandle indexBufferHandle) = 0;

        virtual void Destroy(VertexBufferHandle vertexBufferHandle) = 0;
        virtual void Destroy(IndexBufferHandle indexBufferHandle) = 0;

    public:
        static Ref<RendererContext> Create();
    };
}
