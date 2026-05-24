#pragma once

#include <Quelos/Scenes/Components.h>

#include "Buffer.h"
#include "FrameBuffer.h"
#include "PipelineState.h"
#include "RendererContext.h"
#include "Quelos/Core/Event.h"
#include "Quelos/Renderer/RendererAPI.h"
#include "Shader.h"
#include "VertexBufferLayout.h"

namespace Quelos {
    class ImGuiState;
    class Window;

    using RendererContextFactory = Ref<RendererContext>(*)();
    using ImGuiStateFactory = Ref<ImGuiState>(*)();

    struct RendererFactory {
        RendererContextFactory ContextFactory = nullptr;
        ImGuiStateFactory ImGuiStateFactory = nullptr;
    };

    class QS_API Renderer {
    public:
        static bool IsInitialized();
        static void RegisterRendererContext(RendererFactory factory);
        static void Init(const Ref<Window>& window, RendererAPI api);
        static void Shutdown();

        static void OnEvent(Event& event);

        static bool HomogenousDepth();

        static void StartFrame();
        static void StartSceneRender(const float4x4& view, const float4x4& projection);
        static void BeginRenderPass(const BeginRenderPassAttribs& attribs);
        static void EndRenderPass();
        static void EndFrame();

        static void SubmitMesh(const MeshRenderer& mesh, const WorldTransform& transform);
        static void DrawIndexed(const DrawIndexedAttribs& attribs);

        // Renderer Context API
        static ShaderHandle CreateShader(const ShaderCreateInfo& createInfo);
        static void Destroy(ShaderHandle shaderHandle);

        static PipelineStateHandle CreatePipelineState(const GraphicsPipelineStateCreateInfo& pipelineStateCreateInfo);
        static void BindStaticVariableByName(
            PipelineStateHandle pipelineStateHandle, ShaderType shaderType, std::string_view name,
            GPUBufferHandle bufferHandle
        );
        static void BindPipelineState(PipelineStateHandle pipelineHandle);
        static void Destroy(PipelineStateHandle pipelineStateHandle);

        static GraphicsShaderHandle CreateShader(Buffer vertex, Buffer fragment, const std::string& name);
        static bool RecreateShader(GraphicsShaderHandle handle, Buffer vertex, Buffer fragment);
        static void Submit(GraphicsShaderHandle handle, uint32_t view);
        static void Destroy(GraphicsShaderHandle shaderHandle);

        static GPUBufferHandle CreateBuffer(const GPUBufferSpec& bufferSpec, BufferView bufferView);
        static void UpdateBuffer(GPUBufferHandle bufferHandle, uint64_t offset, BufferView data);
        static void Destroy(GPUBufferHandle bufferHandle);

        static ShaderResourceBindingHandle CreateShaderResourceBinding(
            PipelineStateHandle pipelineStateHandle, bool initStaticResources
        );
        static void BindVariableByName(
            ShaderType shaderType, ShaderResourceBindingHandle srb, std::string_view str, GPUBufferHandle instanceBuffer
        );

        static void CommitShaderResources(ShaderResourceBindingHandle shaderResourceBindingHandle);

        static void Map(GPUBufferHandle bufferHandle, MapType mapType, MapFlags discard, void*& data);
        static void Unmap(GPUBufferHandle bufferHandle, MapType mapType);

        static void Destroy(ShaderResourceBindingHandle shaderResourceBindingHandle);

        static VertexBufferHandle CreateVertexBuffer(BufferView vertices, const VertexLayout& bufferLayout);
        static void BindVertexBuffer(VertexBufferHandle handle, uint32_t stream);
        static void Destroy(VertexBufferHandle vertexBufferHandle);

        static IndexBufferHandle CreateIndexBuffer(Span<uint16_t> indices);
        static void BindIndexBuffer(IndexBufferHandle handle);
        static void Destroy(IndexBufferHandle indexBufferHandle);

        static UniformBufferHandle CreateUniformBuffer(
            const std::string& name, UniformBufferType uniformType, uint32_t count = 1
        );
        static void SetUniformData(UniformBufferHandle uniformBufferHandle, const void* data, uint32_t count = 1);
        static void Destroy(UniformBufferHandle uniformBufferHandle);

        // Texture
        static TextureHandle CreateTexture(const TextureSpecification& spec);
        static TextureHandle CreateTexture(const TextureSpecification& spec, Buffer data);
        static TextureHandle CreateTexture(const TextureSpecification& spec, const std::filesystem::path& path);

        static bool TextureIsVFlipped();

        static const TextureSpecification* GetSpecification(TextureHandle handle);
        static uint64_t TextureGetNativeHandle(TextureHandle handle);
        static void TextureResize(TextureHandle textureHandle, uint32_t width, uint32_t height);

        static void Bind(TextureHandle textureHandle);
        static void Destroy(TextureHandle textureHandle);

        // Render Pass
        static RenderPassHandle CreateRenderPass(const RenderPassSpec& renderPassSpec);
        static void Destroy(RenderPassHandle renderPassHandle);

        // Frame Buffer
        static FrameBufferHandle CreateFrameBuffer(const FrameBufferSpec& frameBufferSpec);

        static uint32_t FrameBufferGetWidth(FrameBufferHandle frameBufferHandle);
        static uint32_t FrameBufferGetHeight(FrameBufferHandle frameBufferHandle);
        static Extent2D FrameBufferGetSize(FrameBufferHandle frameBufferHandle);

        static void FrameBufferResize(FrameBufferHandle frameBufferHandle, uint32_t width, uint32_t height);

        static void Destroy(FrameBufferHandle frameBufferHandle);

        static void IncRef(Handle<Texture> textureHandle);
        static void DecRef(Handle<Texture> textureHandle);

        static void IncRef(Handle<FrameBuffer> textureHandle);
        static void DecRef(Handle<FrameBuffer> textureHandle);

        static void IncRef(Handle<IndexBuffer> textureHandle);
        static void DecRef(Handle<IndexBuffer> textureHandle);

        static void IncRef(Handle<VertexBuffer> textureHandle);
        static void DecRef(Handle<VertexBuffer> textureHandle);

        static void IncRef(Handle<GPUBuffer> textureHandle);
        static void DecRef(Handle<GPUBuffer> textureHandle);
        static void IncRef(Handle<RenderPass> textureHandle);
        static void DecRef(Handle<RenderPass> textureHandle);
        static void IncRef(Handle<ShaderResourceBinding> textureHandle);
        static void DecRef(Handle<ShaderResourceBinding> textureHandle);
        static void IncRef(Handle<PipelineStateObject> textureHandle);
        static void DecRef(Handle<PipelineStateObject> textureHandle);

        static bool IsAlive(PipelineStateHandle pipelineStateHandle);
    };
}
