#pragma once
#include "Quelos/Renderer/RendererContext.h"

namespace Quelos {
    class bgfxRendererContext : public RendererContext {
    public:
        void Init(const Ref<Window>& window, RendererAPI api) override;
        void Shutdown() override;

        ShaderHandle CreateShader(const std::string& filePathVertex, const std::string& filePathFragment) override;
        void Submit(ShaderHandle shaderHandle, uint32_t view) override;
        void Destroy(ShaderHandle shaderHandle) override;

        VertexBufferHandle CreateVertexBuffer(const std::vector<PosColorVertex>& vertices) override;
        void BindVertexBuffer(VertexBufferHandle vertexBufferHandle, uint32_t stream) override;
        void Destroy(VertexBufferHandle vertexBufferHandle) override;

        IndexBufferHandle CreateIndexBuffer(const std::vector<uint16_t>& indices) override;
        void BindIndexBuffer(IndexBufferHandle indexBufferHandle) override;
        void Destroy(IndexBufferHandle indexBufferHandle) override;
    };
}
