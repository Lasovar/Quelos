#include "Renderer.h"

#include "Quelos/Core/Window.h"

#include "FrameBuffer.h"
#include "IndexBuffer.h"
#include "RendererContext.h"
#include "Shader.h"
#include "VertexBuffer.h"

#include "Quelos/Core/Application.h"
#include "Quelos/Core/Events/WindowEvents.h"
#include "Quelos/ImGui/ImGuiState.h"

#include "Quelos/Math/Math.h"
#include "Quelos/Core/Profiling.h"

namespace Quelos {
    static Ref<Window> s_Window;
    static Ref<Time> s_Time;

    static bool s_NeedReset = false;
    static bool s_IsInitialized = false;

    static Ref<RendererContext> s_RendererContext;

    static RendererFactory s_RendererContextFactory;

    static UniformBufferHandle u_lightDir;
    static UniformBufferHandle u_lightColor;
    static UniformBufferHandle u_cameraPos;

    static UniformBufferHandle u_bandCount;
    static UniformBufferHandle u_shadowThreshold;

    // For ramp texture
    static UniformBufferHandle u_rampTex;

    // Per mesh data
    static UniformBufferHandle u_Color;

    bool Renderer::IsInitialized() { return s_IsInitialized; }

    void Renderer::RegisterRendererContext(const RendererFactory factory) {
        s_RendererContextFactory = factory;
        ImGuiState::Register(s_RendererContextFactory.ImGuiStateFactory);
    }

    void Renderer::Init(const Ref<Window>& window, const RendererAPI api) {
        s_Window = window;
        s_Time = Application::Get().GetTime();

        if (!s_RendererContextFactory.ContextFactory) {
            return;
        }

        s_RendererContext = s_RendererContextFactory.ContextFactory();
        if (!s_RendererContext) {
            return;
        }

        s_RendererContext->Init(window, api);

        s_IsInitialized = true;

        /*
        u_lightDir = CreateUniformBuffer("u_lightDir", UniformBufferType::Float4);
        u_lightColor = CreateUniformBuffer("u_lightColor", UniformBufferType::Float4);
        u_cameraPos = CreateUniformBuffer("u_cameraPos", UniformBufferType::Float4);

        u_bandCount = CreateUniformBuffer("u_bandCount", UniformBufferType::Float4);
        u_shadowThreshold = CreateUniformBuffer("u_shadowThreshold", UniformBufferType::Float4);

        u_rampTex = CreateUniformBuffer("s_rampTex", UniformBufferType::Sampler);

        u_Color = CreateUniformBuffer("u_Color", UniformBufferType::Float4);
    */
    }

    RendererAPI Renderer::GetRendererAPI() {
        return s_RendererContext->GetRendererAPI();
    }

    void Renderer::StartFrame() {
        if (s_NeedReset) {
            s_RendererContext->Reset(s_Window->GetWidth(), s_Window->GetHeight());
            s_NeedReset = false;
        }

        s_RendererContext->StartFrame();
    }

    void Renderer::StartSceneRender(const float4x4& view, const float4x4& projection) {
        s_RendererContext->StartSceneRender(view, projection);

        float4 lightDir = math::normalize(-float4(0.5f, -1.0f, 0.3f, 0.0f));
        SetUniformData(u_lightDir, math::value_ptr(lightDir));

        float4 lightColorData{1.0f, 1.0f, 1.0f, 0.0f};
        SetUniformData(u_lightColor, math::value_ptr(lightColorData));

        float4x4 invView = math::inverse(view);
        const float4& camPos(invView[3]);
        SetUniformData(u_cameraPos, math::value_ptr(camPos));

        const float4 bandData{4.0f, 0.0f, 0.0f, 0.0f};
        SetUniformData(u_bandCount, math::value_ptr(bandData));

        float4 shadowData{0.25f, 0.0f, 0.0f, 0.0f};
        SetUniformData(u_shadowThreshold, math::value_ptr(shadowData));
    }

    void Renderer::BeginRenderPass(const BeginRenderPassAttribs& attribs) {
        s_RendererContext->BeginRenderPass(attribs);
    }

    void Renderer::EndRenderPass() {
        s_RendererContext->EndRenderPass();
    }

    void Renderer::EndFrame() {
        QS_PROFILE_SCOPED();
        s_RendererContext->EndFrame();
    }

    void Renderer::SubmitMesh(const MeshRenderer& mesh, const WorldTransform& transform) {
        s_RendererContext->SubmitMesh(mesh, transform);
    }

    void Renderer::DrawIndexed(const DrawIndexedAttribs& attribs) {
        s_RendererContext->DrawIndexed(attribs);
    }

    ShaderHandle Renderer::CreateShader(const ShaderCreateInfo& createInfo) {
        return s_RendererContext->CreateShader(createInfo);
    }

    void Renderer::Destroy(const ShaderHandle shaderHandle) {
        s_RendererContext->Destroy(shaderHandle);
    }

    PipelineStateHandle Renderer::CreatePipelineState(const GraphicsPipelineStateCreateInfo& pipelineStateCreateInfo) {
        return s_RendererContext->CreatePipelineState(pipelineStateCreateInfo);
    }

    void Renderer::BindStaticVariableByName(
        const PipelineStateHandle pipelineStateHandle,
        const ShaderType shaderType,
        const std::string_view name,
        const GpuBufferHandle bufferHandle
    ) {
        s_RendererContext->BindStaticVariableByName(pipelineStateHandle, shaderType, name, bufferHandle);
    }

    void Renderer::BindPipelineState(const PipelineStateHandle pipelineHandle) {
        s_RendererContext->BindPipelineState(pipelineHandle);
    }

    void Renderer::Destroy(const PipelineStateHandle pipelineStateHandle) {
        s_RendererContext->Destroy(pipelineStateHandle);
    }

    PipelineResourceSignatureHandle Renderer::CreatePipelineResourceSignature(
        const PipelineResourceSignatureSpec& pipelineResourceSignatureSpec
    ) {
        return s_RendererContext->CreatePipelineResourceSignature(pipelineResourceSignatureSpec);
    }

    void Renderer::Shutdown() {
        s_RendererContext->Shutdown();
        s_IsInitialized = false;
    }

    void Renderer::OnEvent(Event& event) {
        EventDispatcher dispatcher(event);
        dispatcher.Dispatch<WindowResizeEvent>([](const WindowResizeEvent& _) {
            s_NeedReset = true;
            return false;
        });
    }

    bool Renderer::HomogenousDepth() {
        return s_RendererContext->HomogenousDepth();
    }

    GraphicsShaderHandle Renderer::CreateShader(Buffer vertex, Buffer fragment, const std::string& name) {
        return s_RendererContext->CreateShader(std::move(vertex), std::move(fragment), name);
    }

    bool Renderer::RecreateShader(const GraphicsShaderHandle handle, Buffer vertex, Buffer fragment) {
        return s_RendererContext->RecreateShader(handle, std::move(vertex), std::move(fragment));
    }

    void Renderer::Submit(const GraphicsShaderHandle handle, const uint32_t view) {
        s_RendererContext->Submit(handle, view);
    }

    void Renderer::Destroy(const GraphicsShaderHandle shaderHandle) {
        s_RendererContext->Destroy(shaderHandle);
    }

    GpuBufferHandle Renderer::CreateBuffer(const GPUBufferSpec& bufferSpec, const BufferView bufferView) {
        return s_RendererContext->CreateBuffer(bufferSpec, bufferView);
    }

    void Renderer::UpdateBuffer(const GpuBufferHandle bufferHandle, const uint64_t offset, const BufferView data) {
        s_RendererContext->UpdateBuffer(bufferHandle, offset, data);
    }

    void Renderer::Destroy(const GpuBufferHandle bufferHandle) {
        s_RendererContext->Destroy(bufferHandle);
    }

    ShaderResourceBindingHandle Renderer::CreateShaderResourceBinding(
        const PipelineStateHandle pipelineStateHandle, const bool initStaticResources
    ) {
        return s_RendererContext->CreateShaderResourceBinding(pipelineStateHandle, initStaticResources);
    }

    void Renderer::BindVariableByName(
        const ShaderType shaderType,
        const ShaderResourceBindingHandle srb,
        const std::string_view str,
        const GpuBufferHandle instanceBuffer
    ) {
        s_RendererContext->BindVariableByName(shaderType, srb, str, instanceBuffer);
    }

    void Renderer::BindArrayByName(
        const ShaderType shaderType,
        const ShaderResourceBindingHandle srb,
        const std::string_view name,
        const Span32<const uint64_t> nativeTextureHandles
    ) {
        s_RendererContext->BindArrayByName(shaderType, srb, name, nativeTextureHandles);
    }

    void Renderer::CommitShaderResources(
        const ShaderResourceBindingHandle shaderResourceBindingHandle,
        const ResourceStateTransitionMode resourceStateTransitionMode
    ) {
        s_RendererContext->CommitShaderResources(shaderResourceBindingHandle, resourceStateTransitionMode);
    }

    VertexBufferHandle Renderer::CreateVertexBuffer(const BufferView vertices, const VertexLayout& bufferLayout) {
        return s_RendererContext->CreateVertexBuffer(vertices, bufferLayout);
    }

    void Renderer::BindVertexBuffer(const VertexBufferHandle handle, const uint32_t stream) {
        s_RendererContext->BindVertexBuffer(handle, stream);
    }

    void Renderer::Destroy(const VertexBufferHandle vertexBufferHandle) {
        s_RendererContext->Destroy(vertexBufferHandle);
    }

    IndexBufferHandle Renderer::CreateIndexBuffer(const Span<uint16_t> indices) {
        return s_RendererContext->CreateIndexBuffer(indices);
    }

    void Renderer::BindIndexBuffer(const IndexBufferHandle handle) {
        s_RendererContext->BindIndexBuffer(handle);
    }

    void Renderer::Destroy(const IndexBufferHandle indexBufferHandle) {
        s_RendererContext->Destroy(indexBufferHandle);
    }

    UniformBufferHandle Renderer::CreateUniformBuffer(
        const std::string& name,
        const UniformBufferType uniformType,
        const uint32_t count
    ) {
        return s_RendererContext->CreateUniformBuffer(name, uniformType, count);
    }

    void Renderer::SetUniformData(
        const UniformBufferHandle uniformBufferHandle, const void* data, const uint32_t count
    ) {
        s_RendererContext->SetUniformData(uniformBufferHandle, data, count);
    }

    void Renderer::Destroy(const UniformBufferHandle uniformBufferHandle) {
        s_RendererContext->Destroy(uniformBufferHandle);
    }

    TextureHandle Renderer::CreateTexture(const TextureSpecification& spec) {
        return s_RendererContext->CreateTexture(spec);
    }

    TextureHandle Renderer::CreateTexture(const TextureSpecification& spec, const BufferView data) {
        return s_RendererContext->CreateTexture(spec, data);
    }

    TextureHandle Renderer::CreateTexture(const TextureSpecification& spec, const OsPath& path) {
        return s_RendererContext->CreateTexture(spec, path);
    }

    bool Renderer::TextureIsVFlipped() {
        return s_RendererContext->TextureIsVFlipped();
    }

    TextureViewHandle Renderer::GetTextureView(const TextureHandle textureHandle, const TextureViewType textureView) {
        return s_RendererContext->GetTextureView(textureHandle, textureView);
    }

    const TextureSpecification* Renderer::GetSpecification(const TextureHandle handle) {
        return s_RendererContext->GetSpecification(handle);
    }

    uint64_t Renderer::TextureGetNativeHandle(const TextureHandle handle) {
        return s_RendererContext->TextureGetNativeHandle(handle);
    }

    void Renderer::TextureResize(const TextureHandle textureHandle, const uint32_t width, const uint32_t height) {
        s_RendererContext->TextureResize(textureHandle, width, height);
    }

    void Renderer::CopyTexture(const CopyTextureAttribs& copy) {
        s_RendererContext->CopyTexture(copy);
    }

    void Renderer::MapTextureSubresource(
        const TextureHandle textureHandle,
        const uint32_t mipLevel,
        const uint32_t arraySlice,
        const MapType mapType,
        const MapFlags mapFlags,
        MappedTextureSubresource& mappedData
    ) {
        s_RendererContext->MapTextureSubresource(textureHandle, mipLevel, arraySlice, mapType, mapFlags, mappedData);
    }

    void Renderer::UnmapTextureSubresource(
        const TextureHandle textureHandle,
        const uint32_t mipLevel,
        const uint32_t arraySlice
    ) {
        s_RendererContext->UnmapTextureSubresource(textureHandle, mipLevel, arraySlice);
    }

    void Renderer::Bind(const TextureHandle textureHandle) {
        s_RendererContext->Bind(textureHandle);
    }

    void Renderer::Destroy(const TextureHandle textureHandle) {
        s_RendererContext->Destroy(textureHandle);
    }

    RenderPassHandle Renderer::CreateRenderPass(const RenderPassSpec& renderPassSpec) {
        return s_RendererContext->CreateRenderPass(renderPassSpec);
    }

    void Renderer::Destroy(const RenderPassHandle renderPassHandle) {
        s_RendererContext->Destroy(renderPassHandle);
    }

    FrameBufferHandle Renderer::CreateFrameBuffer(const FrameBufferSpec& frameBufferSpec) {
        return s_RendererContext->CreateFrameBuffer(frameBufferSpec);
    }

    uint32_t Renderer::FrameBufferGetWidth(const FrameBufferHandle frameBufferHandle) {
        return s_RendererContext->FrameBufferGetWidth(frameBufferHandle);
    }

    uint32_t Renderer::FrameBufferGetHeight(const FrameBufferHandle frameBufferHandle) {
        return s_RendererContext->FrameBufferGetHeight(frameBufferHandle);
    }

    Extent2D Renderer::FrameBufferGetSize(const FrameBufferHandle frameBufferHandle) {
        return s_RendererContext->FrameBufferGetSize(frameBufferHandle);
    }

    void Renderer::FrameBufferResize(
        const FrameBufferHandle frameBufferHandle, const uint32_t width, const uint32_t height
    ) {
        s_RendererContext->FrameBufferResize(frameBufferHandle, width, height);
    }

    void Renderer::Destroy(const FrameBufferHandle frameBufferHandle) {
        s_RendererContext->Destroy(frameBufferHandle);
    }

    void Renderer::IncRef(Handle<Texture> textureHandle) { s_RendererContext->IncRef(textureHandle); }
    void Renderer::DecRef(Handle<Texture> textureHandle) { s_RendererContext->DecRef(textureHandle); }
    void Renderer::IncRef(Handle<FrameBuffer> textureHandle) { s_RendererContext->IncRef(textureHandle); }
    void Renderer::DecRef(Handle<FrameBuffer> textureHandle) { s_RendererContext->DecRef(textureHandle); }
    void Renderer::IncRef(Handle<IndexBuffer> textureHandle) { s_RendererContext->IncRef(textureHandle); }
    void Renderer::DecRef(Handle<IndexBuffer> textureHandle) { s_RendererContext->DecRef(textureHandle); }
    void Renderer::IncRef(Handle<VertexBuffer> textureHandle) { s_RendererContext->IncRef(textureHandle); }
    void Renderer::DecRef(Handle<VertexBuffer> textureHandle) { s_RendererContext->DecRef(textureHandle); }
    void Renderer::IncRef(Handle<GpuBuffer> textureHandle) { s_RendererContext->IncRef(textureHandle); }
    void Renderer::DecRef(Handle<GpuBuffer> textureHandle) { s_RendererContext->DecRef(textureHandle); }
    void Renderer::IncRef(Handle<RenderPass> textureHandle) { s_RendererContext->IncRef(textureHandle); }
    void Renderer::DecRef(Handle<RenderPass> textureHandle) { s_RendererContext->DecRef(textureHandle); }
    void Renderer::IncRef(Handle<ShaderResourceBinding> textureHandle) { s_RendererContext->IncRef(textureHandle); }
    void Renderer::DecRef(Handle<ShaderResourceBinding> textureHandle) { s_RendererContext->DecRef(textureHandle); }
    void Renderer::IncRef(Handle<PipelineStateObject> textureHandle) { s_RendererContext->IncRef(textureHandle); }
    void Renderer::DecRef(Handle<PipelineStateObject> textureHandle) { s_RendererContext->DecRef(textureHandle); }

    bool Renderer::IsAlive(PipelineStateHandle pipelineStateHandle) {
        return s_RendererContext->IsAlive(pipelineStateHandle);
    }

    void Renderer::Map(const GpuBufferHandle bufferHandle, const MapType mapType, const MapFlags discard, void*& data) {
        s_RendererContext->Map(bufferHandle, mapType, discard, data);
    }

    void Renderer::Unmap(const GpuBufferHandle bufferHandle, const MapType mapType) {
        s_RendererContext->Unmap(bufferHandle, mapType);
    }

    void Renderer::Destroy(const ShaderResourceBindingHandle shaderResourceBindingHandle) {
        s_RendererContext->Destroy(shaderResourceBindingHandle);
    }
}
