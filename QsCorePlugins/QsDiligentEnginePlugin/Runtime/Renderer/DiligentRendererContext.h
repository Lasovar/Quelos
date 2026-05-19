//
// Created by lasovar on 5/17/26.
//

#pragma once
#include "Quelos/Renderer/Renderer.h"

#if GL_SUPPORTED
#    include "EngineFactoryOpenGL.h"
#endif

#if VULKAN_SUPPORTED
#    include "EngineFactoryVk.h"
#endif

#include "RenderDevice.h"
#include "DeviceContext.h"
#include "SwapChain.h"

#include "RefCntAutoPtr.hpp"

using namespace Diligent;

namespace Quelos {
    class DiligentRendererContext : public RendererContext {
    public:
        void Init(const Ref<Window>& window, RendererAPI api) override;
        bool HomogenousDepth() override;

        void StartFrame() override;
        void EndFrame() override;

        void Reset(uint32_t width, uint32_t height) override;

        void StartSceneRender(FrameBufferHandle frameBuffer, const float4x4& view, const float4x4& projection) override;
        void SubmitMesh(uint32_t viewID, const MeshRenderer& mesh, const WorldTransform& transform) override;

        ShaderHandle CreateShader(Buffer vertex, Buffer fragment, const std::string& name) override;
        bool RecreateShader(ShaderHandle handle, Buffer vertex, Buffer fragment) override;
        void Submit(ShaderHandle shaderHandle, uint32_t view) override;
        void Destroy(ShaderHandle shaderHandle) override;
        VertexBufferHandle CreateVertexBuffer(BufferView vertices, VertexLayout bufferLayout) override;
        void BindVertexBuffer(VertexBufferHandle vertexBufferHandle, uint32_t stream) override;
        void Destroy(VertexBufferHandle vertexBufferHandle) override;
        IndexBufferHandle CreateIndexBuffer(Span<uint16_t> indices) override;
        void BindIndexBuffer(IndexBufferHandle indexBufferHandle) override;
        void Destroy(IndexBufferHandle indexBufferHandle) override;
        UniformBufferHandle
        CreateUniformBuffer(const std::string& name, UniformBufferType uniformType, uint32_t count) override;
        void SetUniformData(UniformBufferHandle uniformBufferHandle, const void* data, uint32_t count) override;
        void Destroy(UniformBufferHandle uniformBufferHandle) override;
        TextureHandle CreateTexture(const TextureSpecification& spec) override;
        TextureHandle CreateTexture(const TextureSpecification& spec, Buffer data) override;
        TextureHandle CreateTexture(const TextureSpecification& spec, const std::filesystem::path& path) override;
        bool TextureIsVFlipped() override;
        void TextureResize(TextureHandle textureHandle, uint32_t width, uint32_t height) override;
        const TextureSpecification* GetSpecification(TextureHandle textureHandle) override;
        uint16_t TextureGetNativeHandle(TextureHandle textureHandle) override;
        void Bind(TextureHandle textureHandle) override;
        void Destroy(TextureHandle textureHandle) override;
        FrameBufferHandle CreateFrameBuffer(uint32_t viewID, Span<TextureHandle> attachments) override;
        uint32_t FrameBufferGetWidth(FrameBufferHandle frameBufferHandle) override;
        uint32_t FrameBufferGetHeight(FrameBufferHandle frameBufferHandle) override;
        uint2 FrameBufferGetSize(FrameBufferHandle frameBufferHandle) override;
        void FrameBufferSetViewID(FrameBufferHandle frameBufferHandle, uint32_t viewId) override;
        uint32_t FrameBufferGetViewID(FrameBufferHandle frameBufferHandle) override;
        void FrameBufferResize(FrameBufferHandle frameBufferHandle, uint32_t width, uint32_t height) override;
        void Bind(FrameBufferHandle frameBufferHandle) override;
        void Destroy(FrameBufferHandle frameBufferHandle) override;
        void Shutdown() override;

        [[nodiscard]] IDeviceContext* GetImmediateContext() const { return m_pImmediateContext.RawPtr(); }
        [[nodiscard]] ISwapChain* GetSwapChain() const { return m_pSwapChain.RawPtr(); }
        [[nodiscard]] IRenderDevice* GetRenderingDevice() const { return m_pDevice; }

    public:
        static DiligentRendererContext& Get() { return *s_Instance; }
    private:
        static DiligentRendererContext* s_Instance;

        RendererAPI m_RendererAPI = RendererAPI::None;
        Ref<Window> m_Window;

        ResourceTable<IBuffer*, VertexBuffer> m_VertexBufferTable;
        ResourceTable<IBuffer*, IndexBuffer> m_IndexBufferTable;

        RefCntAutoPtr<IRenderDevice> m_pDevice;
        RefCntAutoPtr<IDeviceContext> m_pImmediateContext;
        RefCntAutoPtr<ISwapChain> m_pSwapChain;
        RefCntAutoPtr<IPipelineState> m_pPSO;
        RENDER_DEVICE_TYPE m_DeviceType = RENDER_DEVICE_TYPE_GL;
    };
}
