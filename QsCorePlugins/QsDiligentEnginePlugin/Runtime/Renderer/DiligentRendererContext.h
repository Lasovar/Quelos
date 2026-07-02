//
// Created by lasovar on 5/17/26.
//

#pragma once
#include "ObjectBase.hpp"
#include "Quelos/Renderer/Renderer.h"

#if GL_SUPPORTED
#    include "EngineFactoryOpenGL.h"
#endif

#if VULKAN_SUPPORTED
#    include "EngineFactoryVk.h"
#include "vulkan/vulkan.hpp"
#endif

#if D3D11_SUPPORTED
#    include "EngineFactoryD3D11.h"
#endif

#if D3D12_SUPPORTED
#    include "EngineFactoryD3D12.h"
#endif

#include "RenderDevice.h"
#include "DeviceContext.h"
#include "SwapChain.h"

#include "RefCntAutoPtr.hpp"

using namespace Diligent;

namespace Quelos {
    class DataBlob final : public ObjectBase<IDataBlob> {
        ~DataBlob() override;

        IMPLEMENT_QUERY_INTERFACE_IN_PLACE(IID_DataBlob, ObjectBase<IDataBlob>);

        void Resize(size_t NewSize) override {
            m_Buffer = Buffer::Copy(m_Buffer.data(), m_Buffer.size());
        }

        size_t GetSize() const override { return m_Buffer.size(); }
        void* GetDataPtr(size_t Offset) override { return m_Buffer.data() + Offset; }
        const void* GetConstDataPtr(size_t Offset) const override { return m_Buffer.data(); }

    private:
        template <typename AllocatorType, typename ObjectType>
        friend class MakeNewRCObj;

        DataBlob(IReferenceCounters* pRefCounters, Buffer&& DataBuff) noexcept
            : ObjectBase(pRefCounters), m_Buffer(std::move(DataBuff)) {}

    private:
        Buffer m_Buffer;
    };

    struct QTextureData {
        ITexture* Texture = nullptr;
        TextureSpecification Specification;
        Vec<Pair<TextureViewHandle, bool>> TextureViews; // Handle + IsDefaultView
    };

    struct TextureViewData {
        TextureHandle Texture;
        ITextureView* TextureView = nullptr;
        TextureViewSpec Specification;
    };

    struct QFrameBufferData {
        IFramebuffer* FrameBuffer = nullptr;
        std::string Name;
        SmallVec<TextureViewHandle, 2> Attachments;

        FrameBufferSpec Specification;
    };

    struct RenderPassData {
        IRenderPass* RenderPass = nullptr;
        std::string Name;
        SmallVec<AttachmentReference, 4> AttachmentReferences;
        SmallVec<SubPassSpec, 2> SubPasses;
        SmallVec<RenderPassAttachmentSpec, 2> Attachments;
        RenderPassSpec Specification;
    };

    struct QShaderData {
        IShader* Shader = nullptr;
        std::string Name;
        std::string EntryPoint;
        ShaderSpec Specification;
    };

    struct PipelineStateData {
        IPipelineState* PSO = nullptr;
        std::string Name;
        Vec<ShaderResourceVariableSpec> Variables;
        Vec<ImmutableSamplerSpec> ImmutableSamplers;
        Deque<std::string> OwnedStrings;
        PipelineStateSpec PipelineSpec;
        GraphicsPipelineSpec GraphicsPipeline;
    };

    struct QBufferData {
        IBuffer* Buffer;
        std::string Name;
        GpuBufferSpec Specification;
        Vec<GpuBufferViewHandle> BufferViews;
    };

    struct QBufferViewData {
        IBufferView* BufferView;
        GpuBufferHandle Buffer;
    };

    struct PipelineResourceSignatureData {
        IPipelineResourceSignature* Signature = nullptr;
        std::string Name;
        std::string CombinedSamplerSuffix;
        Deque<std::string> OwnedStrings;
        Vec<PipelineResourceSpec> Resources;
        Vec<ImmutableSamplerSpec> ImmutableSamplers;
        PipelineResourceSignatureSpec Specification;
    };

    class DiligentRendererContext : public RendererContext {
    public:
        void Init(const Ref<Window>& window, RendererAPI api) override;
        RendererAPI GetRendererAPI() override;
        bool HomogenousDepth() override;

        void StartFrame() override;
        void EndFrame() override;

        void Reset(uint32_t width, uint32_t height) override;

        void StartSceneRender(const float4x4& view, const float4x4& projection) override;

        void BeginRenderPass(const BeginRenderPassAttribs& beginRenderPassAttrib) override;
        void EndRenderPass() override;

        void SubmitMesh(const MeshRenderer& mesh, const WorldTransform& transform) override;

        void Draw(const DrawAttribs& drawAttribs) override;
        void DrawIndexed(const DrawIndexedAttribs& drawIndexedAttribs) override;

        void DispatchCompute(const DispatchComputeAttribs& dispatchComputeAttribs) override;

        // Shaders
        ShaderHandle CreateShader(const ShaderCreateInfo& createInfo) override;
        void Destroy(ShaderHandle shaderHandle) override;

        PipelineStateHandle CreatePipelineState(
            const GraphicsPipelineStateCreateInfo& pipelineStateCreateInfo
        ) override;

        PipelineStateHandle CreatePipelineState(const ComputePipelineStateCreateInfo& pipelineStateCreateInfo) override;

        PipelineResourceSignatureHandle CreatePipelineResourceSignature(
            const PipelineResourceSignatureSpec& pipelineResourceSignatureSpec
        ) override;
        void Destroy(PipelineResourceSignatureHandle pipelineResourceSignatureHandle) override;

        void BindStaticVariableByName(
            PipelineStateHandle pipelineStateHandle,
            ShaderType shaderType,
            std::string_view name,
            GpuBufferHandle gpuBufferHandle
        ) override;

        void BindStaticVariableByName(
            PipelineStateHandle pipelineStateHandle,
            ShaderType shaderType,
            std::string_view name,
            GpuBufferViewHandle gpuBufferViewHandle
        ) override;

        void BindPipelineState(PipelineStateHandle pipelineStateHandle) override;

        void Destroy(PipelineStateHandle pipelineStateHandle) override;

        GpuBufferHandle CreateBuffer(const GpuBufferSpec& bufferSpec, BufferView data) override;
        GpuBufferViewHandle GetDefaultBufferView(GpuBufferHandle bufferHandle, BufferViewType bufferViewType) override;

        void CopyBuffer(
            GpuBufferHandle srcBuffer, uint64_t srcOffset, ResourceStateTransitionMode srcBufferTransitionMode,
            GpuBufferHandle dstBuffer, uint64_t dstOffset, uint64_t size,
            ResourceStateTransitionMode dstBufferTransitionMode
        ) override;

        ShaderResourceBindingHandle CreateShaderResourceBinding(
            PipelineStateHandle pipelineStateHandle,
            bool initStaticResources
        ) override;

        void BindVariableByName(
            ShaderType shaderType,
            ShaderResourceBindingHandle shaderResourceBindingHandle,
            std::string_view name, GpuBufferViewHandle gpuBufferViewHandle, SetShaderResourceFlag flags
        ) override;

        void BindVariableByName(
            ShaderType shaderType,
            ShaderResourceBindingHandle shaderResourceBindingHandle,
            std::string_view name,
            TextureViewHandle textureViewHandle, SetShaderResourceFlag flags
        ) override;

        void BindArrayByName(
            ShaderType shaderType,
            ShaderResourceBindingHandle shaderResourceBindingHandle,
            std::string_view name,
            Span32<const uint64_t> nativeTextureHandles, SetShaderResourceFlag flags
        ) override;

        void Map(GpuBufferHandle bufferHandle, MapType mapType, MapFlags mapFlags, void*& mappedData) override;
        void Unmap(GpuBufferHandle bufferHandle, MapType mapType) override;

        void CommitShaderResources(ShaderResourceBindingHandle shaderResourceBindingHandle, ResourceStateTransitionMode resourceStateTransitionMode) override;

        void Destroy(ShaderResourceBindingHandle shaderResourceBindingHandle) override;

        void UpdateBuffer(GpuBufferHandle gpuBufferHandle, uint64_t offset, BufferView data) override;
        void Destroy(GpuBufferHandle bufferHandle) override;

        void TransitionShaderResources(ShaderResourceBindingHandle shaderResourceBindingHandle) override;
        void TransitionResource(TextureHandle textureHandle, ResourceState resourceState) override;

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
        TextureHandle CreateTexture(const TextureSpecification& spec, BufferView data) override;
        TextureHandle CreateTexture(const TextureSpecification& spec, const std::filesystem::path& path) override;

        bool TextureIsVFlipped() override;
        void TextureResize(TextureHandle textureHandle, uint32_t width, uint32_t height) override;

        const TextureSpecification* GetSpecification(TextureHandle textureHandle) override;
        uint64_t TextureGetNativeHandle(TextureHandle textureHandle) override;

        TextureHandle GetTexture(TextureViewHandle textureView) override;

        TextureViewHandle TextureGetView(TextureHandle texture, TextureViewType textureViewType) override;
        TextureViewHandle TextureCreateView(TextureHandle texture, TextureViewSpec textureViewSpec) override;

        void CopyTexture(const CopyTextureAttribs& copyAttribs) override;

        void MapTextureSubresource(
            TextureHandle textureHandle,
            uint32_t mipLevel,
            uint32_t arraySlice,
            MapType mapType,
            MapFlags mapFlags,
            MappedTextureSubresource& mappedData
        ) override;

        void UnmapTextureSubresource(TextureHandle textureHandle, uint32_t mipLevel, uint32_t arraySlice) override;

        void Bind(TextureHandle textureHandle) override;

        void Destroy(TextureHandle textureHandle) override;

        // Render Pass
        RenderPassHandle CreateRenderPass(const RenderPassSpec& renderPassSpec) override;
        void Destroy(RenderPassHandle renderPassHandle) override;

        FrameBufferHandle CreateFrameBuffer(const FrameBufferSpec& frameBufferSpec) override;
        uint32_t FrameBufferGetWidth(FrameBufferHandle frameBufferHandle) override;
        uint32_t FrameBufferGetHeight(FrameBufferHandle frameBufferHandle) override;
        Extent2D FrameBufferGetSize(FrameBufferHandle frameBufferHandle) override;
        void FrameBufferResize(FrameBufferHandle frameBufferHandle, uint32_t width, uint32_t height) override;
        void Destroy(FrameBufferHandle frameBufferHandle) override;
        void Shutdown() override;

        [[nodiscard]] IDeviceContext* GetImmediateContext() const { return m_pImmediateContext.RawPtr(); }
        [[nodiscard]] ISwapChain* GetSwapChain() const { return m_pSwapChain.RawPtr(); }
        [[nodiscard]] IRenderDevice* GetRenderingDevice() const { return m_pDevice; }

    public:
        static DiligentRendererContext& Get() { return *s_Instance; }

        void IncRef(Handle<Texture> textureHandle) override;
        void DecRef(Handle<Texture> textureHandle) override;
        void IncRef(Handle<FrameBuffer> textureHandle) override;
        void DecRef(Handle<FrameBuffer> textureHandle) override;
        void IncRef(Handle<IndexBuffer> textureHandle) override;
        void DecRef(Handle<IndexBuffer> textureHandle) override;
        void IncRef(Handle<VertexBuffer> textureHandle) override;
        void DecRef(Handle<VertexBuffer> textureHandle) override;
        void IncRef(Handle<GpuBuffer> textureHandle) override;
        void DecRef(Handle<GpuBuffer> textureHandle) override;
        void IncRef(Handle<RenderPass> textureHandle) override;
        void DecRef(Handle<RenderPass> textureHandle) override;
        void IncRef(Handle<ShaderResourceBinding> srb) override;
        void DecRef(Handle<ShaderResourceBinding> srb) override;
        void IncRef(Handle<PipelineStateObject> pso) override;
        void DecRef(Handle<PipelineStateObject> pso) override;

        bool IsAlive(PipelineStateHandle pipelineStateHandle) override;

    private:
        static DiligentRendererContext* s_Instance;

        RendererAPI m_RendererAPI = RendererAPI::None;
        Ref<Window> m_Window;

        ResourceTable<IBuffer*, VertexBuffer> m_VertexBufferTable;
        ResourceTable<IBuffer*, IndexBuffer> m_IndexBufferTable;
        ResourceTable<QShaderData, Shader> m_ShaderTable;
        ResourceTable<QTextureData, Texture> m_TextureTable;
        ResourceTable<TextureViewData, TextureView> m_TextureViewTable;
        ResourceTable<RenderPassData, RenderPass> m_RenderPassTable;
        ResourceTable<QFrameBufferData, FrameBuffer> m_FrameBufferTable;
        ResourceTable<QBufferData, GpuBuffer> m_BufferTable;
        ResourceTable<QBufferViewData, GpuBufferView> m_BufferViewTable;
        ResourceTable<PipelineStateData, PipelineStateObject> m_PipelineStateTable;
        ResourceTable<IShaderResourceBinding*, ShaderResourceBinding> m_ShaderResourceBindingTable;
        ResourceTable<PipelineResourceSignatureData, PipelineResourceSignature> m_PipelineResourceSignatureTable;

        RefCntAutoPtr<IRenderDevice> m_pDevice;
        RefCntAutoPtr<IDeviceContext> m_pImmediateContext;
        RefCntAutoPtr<ISwapChain> m_pSwapChain;
        RefCntAutoPtr<IShaderResourceBinding> m_pSRB;
    };
}
