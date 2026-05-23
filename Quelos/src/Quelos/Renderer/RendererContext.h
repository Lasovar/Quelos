#pragma once

#include "FrameBuffer.h"
#include "IndexBuffer.h"
#include "RendererAPI.h"
#include "RenderPass.h"
#include "RenderPassAttrib.h"
#include "Shader.h"
#include "Texture.h"
#include "UniformBuffer.h"
#include "VertexBuffer.h"
#include "VertexBufferLayout.h"
#include "Quelos/Core/Window.h"
#include "Quelos/Scenes/Components.h"

namespace Quelos {
    class QS_API RendererContext {
    public:
        virtual void Init(const Ref<Window>& ref, RendererAPI api) = 0;

        virtual ~RendererContext() = default;

        virtual bool HomogenousDepth() = 0;

        virtual void StartFrame() = 0;
        virtual void EndFrame() = 0;

        virtual void StartSceneRender(
            FrameBufferHandle frameBuffer,
            const float4x4& view,
            const float4x4& projection
        ) = 0;

        virtual void BeginRenderPass(const BeginRenderPassAttribs& beginRenderPassAttrib) = 0;
        virtual void EndRenderPass() = 0;

        virtual void SubmitMesh(const MeshRenderer& mesh, const WorldTransform& transform) = 0;

        virtual void Reset(uint32_t width, uint32_t height) = 0;

        virtual ShaderHandle CreateShader(Buffer vertex, Buffer fragment, const std::string& name) = 0;
        virtual bool RecreateShader(ShaderHandle handle, Buffer vertex, Buffer fragment) = 0;
        virtual void Submit(ShaderHandle shaderHandle, uint32_t view) = 0;
        virtual void Destroy(ShaderHandle shaderHandle) = 0;

        virtual VertexBufferHandle CreateVertexBuffer(BufferView vertices, VertexLayout bufferLayout) = 0;
        virtual void BindVertexBuffer(VertexBufferHandle vertexBufferHandle, uint32_t stream) = 0;
        virtual void Destroy(VertexBufferHandle vertexBufferHandle) = 0;

        virtual IndexBufferHandle CreateIndexBuffer(Span<uint16_t> vertices) = 0;
        virtual void BindIndexBuffer(IndexBufferHandle indexBufferHandle) = 0;
        virtual void Destroy(IndexBufferHandle indexBufferHandle) = 0;

        virtual UniformBufferHandle CreateUniformBuffer(const std::string& name, UniformBufferType uniformType, uint32_t count = 1) = 0;
        virtual void SetUniformData(UniformBufferHandle uniformBufferHandle, const void* data, uint32_t count = 1) = 0;
        virtual void Destroy(UniformBufferHandle uniformBufferHandle) = 0;

        // Texture
        virtual TextureHandle CreateTexture(const TextureSpecification& spec) = 0;
        virtual TextureHandle CreateTexture(const TextureSpecification& spec, Buffer data) = 0;
        virtual TextureHandle CreateTexture(const TextureSpecification& spec, const std::filesystem::path& path) = 0;

        virtual bool TextureIsVFlipped() = 0;

        virtual void TextureResize(TextureHandle textureHandle, uint32_t width, uint32_t height) = 0;
        virtual const TextureSpecification* GetSpecification(TextureHandle textureHandle) = 0;
        virtual uint64_t TextureGetNativeHandle(TextureHandle textureHandle) = 0;

        virtual void Bind(TextureHandle textureHandle) = 0;
        virtual void Destroy(TextureHandle textureHandle) = 0;

        // Render Pass
        virtual RenderPassHandle CreateRenderPass(const RenderPassSpec& renderPassSpec) = 0;
        virtual void Destroy(RenderPassHandle renderPassHandle) = 0;

        // Frame Buffer
        virtual FrameBufferHandle CreateFrameBuffer(const FrameBufferSpec& frameBufferSpec) = 0;

        virtual uint32_t FrameBufferGetWidth(FrameBufferHandle frameBufferHandle) = 0;
        virtual uint32_t FrameBufferGetHeight(FrameBufferHandle frameBufferHandle) = 0;
        virtual Extent2D FrameBufferGetSize(FrameBufferHandle frameBufferHandle) = 0;

        virtual void FrameBufferResize(FrameBufferHandle frameBufferHandle, uint32_t width, uint32_t height) = 0;

        virtual void Destroy(FrameBufferHandle frameBufferHandle) = 0;

        virtual void Shutdown() = 0;
    };
}
