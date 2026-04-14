#pragma once
#include "bgfx/bgfx.h"
#include "Quelos/Renderer/RendererContext.h"

namespace Quelos {
    class QS_API bgfxRendererContext : public RendererContext {
    public:
        void Init(const Ref<Window>& window, RendererAPI api) override;

        bool HomogenousDepth() override;

        void StartFrame() override;
        void EndFrame() override;

        void StartSceneRender(
            FrameBufferHandle frameBuffer,
            const glm::mat4& view,
            const glm::mat4& projection
        ) override;

        void SubmitMesh(uint32_t viewID, const MeshComponent& mesh, const WorldTransform& transform) override;

        void Reset(uint32_t width, uint32_t height) override;

        void Shutdown() override;

        ShaderHandle CreateShader(const std::string& filePathVertex, const std::string& filePathFragment) override;
        void Submit(ShaderHandle shaderHandle, uint32_t view) override;
        void Destroy(ShaderHandle shaderHandle) override;

        VertexBufferHandle CreateVertexBuffer(BufferView vertices, VertexLayout bufferLayout) override;
        void BindVertexBuffer(VertexBufferHandle vertexBufferHandle, uint32_t stream) override;
        void Destroy(VertexBufferHandle vertexBufferHandle) override;

        IndexBufferHandle CreateIndexBuffer(const std::vector<uint16_t>& indices) override;
        void BindIndexBuffer(IndexBufferHandle indexBufferHandle) override;
        void Destroy(IndexBufferHandle indexBufferHandle) override;

        // Texture
        TextureHandle CreateTexture(const TextureSpecification& spec) override;
        TextureHandle CreateTexture(const TextureSpecification& spec, Buffer data) override;
        TextureHandle CreateTexture(const TextureSpecification& spec, const std::filesystem::path& path) override;

        bool TextureIsVFlipped() override;

        void TextureResize(TextureHandle textureHandle, uint32_t width, uint32_t height) override;
        uint16_t TextureGetNativeHandle(TextureHandle textureHandle) override;
        const TextureSpecification* GetSpecification(TextureHandle textureHandle) override;

        void Bind(TextureHandle textureHandle) override;
        void Destroy(TextureHandle textureHandle) override;

        FrameBufferHandle CreateFrameBuffer(uint32_t viewID, Span<TextureHandle> attachments) override;
        uint32_t FrameBufferGetWidth(FrameBufferHandle frameBufferHandle) override;
        uint32_t FrameBufferGetHeight(FrameBufferHandle frameBufferHandle) override;
        glm::uvec2 FrameBufferGetSize(FrameBufferHandle frameBufferHandle) override;
        void FrameBufferSetViewID(FrameBufferHandle frameBufferHandle, uint32_t viewId) override;
        uint32_t FrameBufferGetViewID(FrameBufferHandle frameBufferHandle) override;
        void FrameBufferResize(FrameBufferHandle frameBufferHandle, uint32_t width, uint32_t height) override;
        void Bind(FrameBufferHandle frameBufferHandle) override;
        void Destroy(FrameBufferHandle frameBufferHandle) override;

        static bgfx::TextureHandle GetBgfxTextureHandle(TextureHandle handle);
    };
}
