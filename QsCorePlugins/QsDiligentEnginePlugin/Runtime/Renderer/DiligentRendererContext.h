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
    };

    struct QFrameBufferData {
        IFramebuffer* FrameBuffer;
        std::string Name;
        SmallVec<TextureHandle, 2> Attachments;

        FrameBufferSpec Specification;
    };

    struct RenderPassData {
        IRenderPass* RenderPass;
        std::string Name;
        SmallVec<AttachmentReference, 4> AttachmentReferences;
        SmallVec<SubPassSpec, 2> SubPasses;
        SmallVec<RenderPassAttachmentSpec, 2> Attachments;
        RenderPassSpec Specification;
    };

    struct ShaderData {
        IShader* Shader;
        std::string Name;
        std::string EntryPoint;
        ShaderSpec Specification;
    };

    struct PipelineStateData {
        IPipelineState* PSO;
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
        GPUBufferSpec Specification;
    };

    class DiligentRendererContext : public RendererContext {
    public:
        void Init(const Ref<Window>& window, RendererAPI api) override;
        bool HomogenousDepth() override;

        void StartFrame() override;
        void EndFrame() override;

        void Reset(uint32_t width, uint32_t height) override;

        void StartSceneRender(const float4x4& view, const float4x4& projection) override;

        void BeginRenderPass(const BeginRenderPassAttribs& beginRenderPassAttrib) override;
        void EndRenderPass() override;

        void SubmitMesh(const MeshRenderer& mesh, const WorldTransform& transform) override;
        void DrawIndexed(const DrawIndexedAttribs& drawIndexedAttribs) override;

        // Shaders
        ShaderHandle CreateShader(const ShaderCreateInfo& createInfo) override;
        void Destroy(ShaderHandle shaderHandle) override;

        PipelineStateHandle CreatePipelineState(
            const GraphicsPipelineStateCreateInfo& pipelineStateCreateInfo
        ) override;

        void BindStaticVariableByName(
            PipelineStateHandle pipelineStateHandle,
            ShaderType shaderType,
            std::string_view name,
            GPUBufferHandle gpuBufferHandle
        ) override;

        void BindPipelineState(PipelineStateHandle pipelineStateHandle) override;

        void Destroy(PipelineStateHandle pipelineStateHandle) override;

        GraphicsShaderHandle CreateShader(Buffer vertex, Buffer fragment, const std::string& name) override;
        bool RecreateShader(GraphicsShaderHandle handle, Buffer vertex, Buffer fragment) override;
        void Submit(GraphicsShaderHandle shaderHandle, uint32_t view) override;
        void Destroy(GraphicsShaderHandle shaderHandle) override;

        GPUBufferHandle CreateBuffer(const GPUBufferSpec& bufferSpec, BufferView data) override;

        ShaderResourceBindingHandle CreateShaderResourceBinding(
            PipelineStateHandle pipelineStateHandle,
            bool initStaticResources
        ) override;

        void BindVariableByName(
            ShaderType shaderType,
            ShaderResourceBindingHandle shaderResourceBindingHandle,
            std::string_view name, GPUBufferHandle gpuBufferHandle
        ) override;

        void Map(GPUBufferHandle bufferHandle, MapType mapType, MapFlags mapFlags, void*& mappedData) override;
        void Unmap(GPUBufferHandle bufferHandle, MapType mapType) override;

        void CommitShaderResources(ShaderResourceBindingHandle shaderResourceBindingHandle) override;

        void Destroy(ShaderResourceBindingHandle shaderResourceBindingHandle) override;

        void UpdateBuffer(GPUBufferHandle gpuBufferHandle, uint64_t offset, BufferView data) override;
        void Destroy(GPUBufferHandle bufferHandle) override;

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
        uint64_t TextureGetNativeHandle(TextureHandle textureHandle) override;
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

    private:
        static DiligentRendererContext* s_Instance;

        RendererAPI m_RendererAPI = RendererAPI::None;
        Ref<Window> m_Window;

        ResourceTable<IBuffer*, VertexBuffer> m_VertexBufferTable;
        ResourceTable<IBuffer*, IndexBuffer> m_IndexBufferTable;
        ResourceTable<ShaderData, Shader> m_ShaderTable;
        ResourceTable<QTextureData, Texture> m_TextureTable;
        ResourceTable<RenderPassData, RenderPass> m_RenderPassTable;
        ResourceTable<QFrameBufferData, FrameBuffer> m_FrameBufferTable;
        ResourceTable<QBufferData, GPUBuffer> m_BufferTable;
        ResourceTable<PipelineStateData, PipelineStateObject> m_PipelineStateTable;
        ResourceTable<IShaderResourceBinding*, ShaderResourceBinding> m_ShaderResourceBindingTable;

        RefCntAutoPtr<IRenderDevice> m_pDevice;
        RefCntAutoPtr<IDeviceContext> m_pImmediateContext;
        RefCntAutoPtr<ISwapChain> m_pSwapChain;
        RefCntAutoPtr<IBuffer> m_VSConstants;
        RefCntAutoPtr<IBuffer> m_VSTransform;
        RefCntAutoPtr<IShaderResourceBinding> m_pSRB;
    };
}
