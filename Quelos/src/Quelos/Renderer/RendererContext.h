#pragma once

#include "GpuBuffer.h"
#include "FrameBuffer.h"
#include "IndexBuffer.h"
#include "PipelineState.h"
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
#include "ShaderResourceBinding.h"

namespace Quelos {
    enum class DrawFlags : uint8_t {
        /// No flags.
        None = 0u,

        /// Verify the state of index and vertex buffers (if any) used by the draw
        /// command. State validation is only performed in debug and development builds
        /// and the flag has no effect in release build.
        VerifyStates = 1u << 0u,

        /// Verify correctness of parameters passed to the draw command.
        ///
        /// \remarks This flag only has effect in debug and development builds.
        ///          Verification is always disabled in release configuration.
        VerifyDrawAttribs = 1u << 1u,

        /// Perform all state validation checks
        ///
        /// \remarks This flag only has effect in debug and development builds.
        ///          Verification is always disabled in release configuration.
        VerifyAll = VerifyStates | VerifyDrawAttribs,

        /// Indicates that none of the dynamic resource buffers used by the draw command
        /// have been modified by the CPU since the last command.
        ///
        /// This flag should be used to improve performance when an application issues a
        /// series of draw commands that use the same pipeline state and shader resources and
        /// no dynamic buffers (constant or bound as shader resources) are updated between the
        /// commands.
        /// Any buffer variable not created with `SHADER_VARIABLE_FLAG_NO_DYNAMIC_BUFFERS` or
        /// `PIPELINE_RESOURCE_FLAG_NO_DYNAMIC_BUFFERS` flags is counted as dynamic.
        /// The flag has no effect on dynamic vertex and index buffers.
        ///
        ///  **Details**
        ///
        ///  D3D12 and Vulkan backends have to perform some work to make data in buffers
        ///  available to draw commands. When a dynamic buffer is mapped, the engine allocates
        ///  new memory and assigns a new GPU address to this buffer. When a draw command is issued,
        ///  this GPU address needs to be used. By default the engine assumes that the CPU may
        ///  map the buffer before any command (to write new transformation matrices for example)
        ///  and that all GPU addresses need to always be refreshed. This is not always the case,
        ///  and the application may use the flag to inform the engine that the data in the buffer
        ///  stay intact to avoid extra work.\n
        ///  Note that after a new PSO is bound or an SRB is committed, the engine will always set all
        ///  required buffer addresses/offsets regardless of the flag. The flag will only take effect
        ///  on the second and subsequent draw calls that use the same PSO and SRB.\n
        ///  The flag has no effect in D3D11 and OpenGL backends.
        ///
        ///  **Implementation details**
        ///
        ///  Vulkan backend allocates `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC` descriptors for all uniform (constant),
        ///  buffers and `VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC` descriptors for storage buffers.
        ///  Note that HLSL structured buffers are mapped to read-only storage buffers in SPIRV and RW buffers
        ///  are mapped to RW-storage buffers.
        ///  By default, all dynamic descriptor sets that have dynamic buffers bound are updated every time a draw command is
        ///  issued (see PipelineStateVkImpl::BindDescriptorSetsWithDynamicOffsets). When `DRAW_FLAG_DYNAMIC_RESOURCE_BUFFERS_INTACT`
        ///  is specified, dynamic descriptor sets are only bound by the first draw command that uses the PSO and the SRB.
        ///  The flag avoids binding descriptors with the same offsets if none of the dynamic offsets have changed.
        ///
        ///  Direct3D12 backend binds constant buffers to root views. By default the engine assumes that virtual GPU addresses
        ///  of all dynamic buffers may change between the draw commands and always binds dynamic buffers to root views
        ///  (see RootSignature::CommitRootViews). When `DRAW_FLAG_DYNAMIC_RESOURCE_BUFFERS_INTACT` is set, root views are only bound
        ///  by the first draw command that uses the PSO + SRB pair. The flag avoids setting the same GPU virtual addresses when
        ///  they stay unchanged.
        ///
        ///  \see DRAW_FLAG_INLINE_CONSTANTS_INTACT, DRAW_FLAG_DYNAMIC_BUFFERS_AND_INLINE_CONSTANTS_INTACT
        DynamicResourceBuffersIntact = 1u << 2u,

        /// Indicates that none of the inline constants used by the draw command
        /// have been modified by the CPU since the last command.
        ///
        /// This flag should be used to improve performance when an application issues a
        /// series of draw commands that use the same pipeline state and shader resources and
        /// no inline constants are updated between the commands.
        ///
        /// \see DRAW_FLAG_DYNAMIC_RESOURCE_BUFFERS_INTACT, DRAW_FLAG_DYNAMIC_BUFFERS_AND_INLINE_CONSTANTS_INTACT
        InlineConstantsIntact = 1u << 3u,

        /// Indicates that neither the dynamic resource buffers nor the inline constants used by
        /// the draw command have been modified by the CPU since the previous command.
        ///
        /// \see DRAW_FLAG_DYNAMIC_RESOURCE_BUFFERS_INTACT, DRAW_FLAG_INLINE_CONSTANTS_INTACT
        DynamicBuffersAndInlineConstantsIntact = DynamicResourceBuffersIntact | InlineConstantsIntact
    };

    struct DrawAttribs {
        /// The number of vertices to draw.
        uint32_t NumVertices = 0;

        /// Additional flags, see Diligent::DRAW_FLAGS.
        DrawFlags Flags = DrawFlags::None;

        /// The number of instances to draw.

        /// If more than one instance is specified, instanced draw call will be performed.
        uint32_t NumInstances = 1;

        /// LOCATION (or INDEX, but NOT the byte offset) of the first vertex in the
        /// vertex buffer to start reading vertices from.
        uint32_t StartVertexLocation = 0;

        /// LOCATION (or INDEX, but NOT the byte offset) in the vertex buffer to start
        /// reading instance data from.
        uint32_t FirstInstanceLocation = 0;
    };

    struct DrawIndexedAttribs {
        /// The number of indices to draw.
        uint32_t NumIndices = 0;

        /// The type of elements in the index buffer.

        /// Allowed values: `UInt` and `UInt16`.
        ValueType IndexType = ValueType::Undefined;

        /// Additional flags, see Diligent::DRAW_FLAGS.
        DrawFlags Flags = DrawFlags::None;

        /// Number of instances to draw.

        /// If more than one instance is specified, instanced draw call will be performed.
        uint32_t NumInstances = 1;

        /// LOCATION (NOT the byte offset) of the first index in
        /// the index buffer to start reading indices from.
        uint32_t FirstIndexLocation = 0;

        /// A constant which is added to each index before accessing the vertex buffer.
        uint32_t BaseVertex = 0;

        /// LOCATION (or INDEX, but NOT the byte offset) in the vertex
        /// buffer to start reading instance data from.
        uint32_t FirstInstanceLocation = 0;
    };

    enum class ResourceStateTransitionMode : uint8_t {
        /// Perform no state transitions and no state validation.
        /// Resource states are not accessed (either read or written) by the command.
        None = 0,

        /// Transition resources to the states required by the specific command.
        /// Resources in unknown state are ignored.
        ///
        /// Any method that uses this mode may alter the state of the resources it works with.
        /// As automatic state management is not thread-safe, no other thread is allowed to read
        /// or write the state of the resources being transitioned.
        /// If the application intends to use the same resources in other threads simultaneously, it needs to
        /// explicitly manage the states using IDeviceContext::TransitionResourceStates() method.
        ///
        /// \note    If a resource is used in multiple threads by multiple contexts, there will be race condition accessing
        ///          internal resource state. An application should use manual resource state management in this case.
        Transition,

        /// Do not transition, but verify that states are correct.
        /// No validation is performed if the state is unknown to the engine.
        /// This mode only has effect in debug and development builds. No validation
        /// is performed in release build.
        ///
        /// \note    Any method that uses this mode will read the state of resources it works with.
        ///          As automatic state management is not thread-safe, no other thread is allowed to alter
        ///          the state of resources being used by the command. It is safe to read these states.
        Verify
    };

    struct CopyTextureAttribs {
        TextureHandle Source;
        TextureHandle Destination;
        ResourceStateTransitionMode SourceTransitionMode = ResourceStateTransitionMode::Transition;
        ResourceStateTransitionMode DestinationTransitionMode = ResourceStateTransitionMode::Transition;
    };

    struct QS_API DispatchComputeAttribs {
        /// The number of groups dispatched in X direction.
        uint32_t ThreadGroupCountX = 1;

        /// The number of groups dispatched in Y direction.
        uint32_t ThreadGroupCountY = 1;

        /// The number of groups dispatched in Z direction.
        uint32_t ThreadGroupCountZ = 1;

        /// Compute group X size.

        /// \remarks
        ///     This member is only used by Metal backend and is ignored by others.
        uint32_t MtlThreadGroupSizeX = 0;

        /// Compute group Y size.

        /// \remarks
        ///     This member is only used by Metal backend and is ignored by others.
        uint32_t MtlThreadGroupSizeY = 0;

        /// Compute group Z size.

        /// \remarks
        ///     This member is only used by Metal backend and is ignored by others.
        uint32_t MtlThreadGroupSizeZ = 0;
    };

    class QS_API RendererContext {
    public:
        virtual void Init(const Ref<Window>& ref, RendererAPI api) = 0;
        virtual RendererAPI GetRendererAPI() = 0;

        virtual ~RendererContext() = default;

        virtual bool HomogenousDepth() = 0;

        virtual void StartFrame() = 0;
        virtual void EndFrame() = 0;

        virtual void StartSceneRender(
            const float4x4& view,
            const float4x4& projection
        ) = 0;

        virtual void BeginRenderPass(const BeginRenderPassAttribs& beginRenderPassAttrib) = 0;
        virtual void EndRenderPass() = 0;

        virtual void SubmitMesh(const MeshRenderer& mesh, const WorldTransform& transform) = 0;

        virtual void Draw(const DrawAttribs& drawAttribs) = 0;
        virtual void DrawIndexed(const DrawIndexedAttribs& drawIndexedAttribs) = 0;

        virtual void DispatchCompute(const DispatchComputeAttribs& dispatchComputeAttribs) = 0;

        virtual void Reset(uint32_t width, uint32_t height) = 0;

        // Shaders
        virtual ShaderHandle CreateShader(const ShaderCreateInfo& createInfo) = 0;
        virtual void Destroy(ShaderHandle shaderHandle) = 0;

        virtual PipelineStateHandle CreatePipelineState(
            const GraphicsPipelineStateCreateInfo& pipelineStateCreateInfo
        ) = 0;

        virtual PipelineStateHandle CreatePipelineState(
            const ComputePipelineStateCreateInfo& pipelineStateCreateInfo
        ) = 0;

        virtual PipelineResourceSignatureHandle CreatePipelineResourceSignature(
            const PipelineResourceSignatureSpec& pipelineResourceSignatureSpec
        ) = 0;

        virtual void Destroy(PipelineResourceSignatureHandle pipelineResourceSignatureHandle) = 0;

        virtual void BindStaticVariableByName(
            PipelineStateHandle pipelineStateHandle,
            ShaderType shaderType,
            std::string_view name,
            GpuBufferHandle gpuBufferHandle
        ) = 0;

        virtual void BindStaticVariableByName(
            PipelineStateHandle pipelineStateHandle,
            ShaderType shaderType,
            std::string_view name,
            GpuBufferViewHandle gpuBufferViewHandle
        ) = 0;

        virtual void BindPipelineState(PipelineStateHandle pipelineStateHandle) = 0;

        virtual void Destroy(PipelineStateHandle pipelineStateHandle) = 0;
        virtual void Destroy(GpuBufferHandle bufferHandle) = 0;

        virtual GpuBufferHandle CreateBuffer(const GpuBufferSpec& bufferSpec, BufferView data) = 0;
        virtual GpuBufferViewHandle GetDefaultBufferView(GpuBufferHandle bufferHandle, BufferViewType bufferViewType) = 0;

        virtual void CopyBuffer(
            GpuBufferHandle srcBuffer, uint64_t srcOffset, ResourceStateTransitionMode srcBufferTransitionMode,
            GpuBufferHandle dstBuffer, uint64_t dstOffset, uint64_t size,
            ResourceStateTransitionMode dstBufferTransitionMode
        ) = 0;

        virtual ShaderResourceBindingHandle CreateShaderResourceBinding(
            PipelineStateHandle pipelineStateHandle,
            bool initStaticResources
        ) = 0;

        virtual void BindVariableByName(
            ShaderType shaderType,
            ShaderResourceBindingHandle shaderResourceBindingHandle,
            std::string_view name, GpuBufferViewHandle gpuBufferHandle
        ) = 0;

        virtual void BindVariableByName(
            ShaderType shaderType,
            ShaderResourceBindingHandle shaderResourceBindingHandle,
            std::string_view name,
            TextureViewHandle textureViewHandle
        ) = 0;

        virtual void BindArrayByName(
            ShaderType shaderType,
            ShaderResourceBindingHandle shaderResourceBindingHandle,
            std::string_view name,
            Span32<const uint64_t> nativeTextureHandles
        ) = 0;

        virtual void Map(GpuBufferHandle bufferHandle, MapType mapType, MapFlags mapFlags, void*& mappedData) = 0;
        virtual void Unmap(GpuBufferHandle bufferHandle, MapType mapType) = 0;

        virtual void CommitShaderResources(ShaderResourceBindingHandle shaderResourceBindingHandle, ResourceStateTransitionMode resourceStateTransitionMode) = 0;

        virtual void Destroy(ShaderResourceBindingHandle shaderResourceBindingHandle) = 0;

        virtual void UpdateBuffer(GpuBufferHandle gpuBufferHandle, uint64_t offset, BufferView data) = 0;

        virtual void TransitionResource(TextureHandle textureHandle, ResourceState resourceState) = 0;

        virtual VertexBufferHandle CreateVertexBuffer(BufferView vertices, VertexLayout bufferLayout) = 0;
        virtual void BindVertexBuffer(VertexBufferHandle vertexBufferHandle, uint32_t stream) = 0;
        virtual void Destroy(VertexBufferHandle vertexBufferHandle) = 0;

        virtual IndexBufferHandle CreateIndexBuffer(Span<uint16_t> vertices) = 0;
        virtual void BindIndexBuffer(IndexBufferHandle indexBufferHandle) = 0;
        virtual void Destroy(IndexBufferHandle indexBufferHandle) = 0;

        virtual UniformBufferHandle CreateUniformBuffer(
            const std::string& name, UniformBufferType uniformType, uint32_t count = 1
        ) = 0;
        virtual void SetUniformData(UniformBufferHandle uniformBufferHandle, const void* data, uint32_t count = 1) = 0;
        virtual void Destroy(UniformBufferHandle uniformBufferHandle) = 0;

        // Texture
        virtual TextureHandle CreateTexture(const TextureSpecification& spec) = 0;
        virtual TextureHandle CreateTexture(const TextureSpecification& spec, BufferView data) = 0;
        virtual TextureHandle CreateTexture(const TextureSpecification& spec, const OsPath& path) = 0;

        virtual bool TextureIsVFlipped() = 0;

        virtual void TextureResize(TextureHandle textureHandle, uint32_t width, uint32_t height) = 0;
        virtual const TextureSpecification* GetSpecification(TextureHandle textureHandle) = 0;
        virtual uint64_t TextureGetNativeHandle(TextureHandle textureHandle) = 0;

        virtual TextureHandle GetTexture(TextureViewHandle textureView) = 0;

        virtual TextureViewHandle TextureGetView(TextureHandle texture, TextureViewType textureViewType) = 0;
        virtual TextureViewHandle TextureCreateView(TextureHandle texture, TextureViewSpec textureViewSpec) = 0;

        virtual void CopyTexture(const CopyTextureAttribs& copyAttribs) = 0;

        virtual void MapTextureSubresource(
            TextureHandle textureHandle,
            uint32_t mipLevel,
            uint32_t arraySlice,
            MapType mapType,
            MapFlags mapFlags,
            MappedTextureSubresource& mappedData
        ) = 0;

        virtual void UnmapTextureSubresource(
            TextureHandle textureHandle,
            uint32_t mipLevel,
            uint32_t arraySlice
        ) = 0;

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

        // Too much boilerplate... maybe a system similar to UntypedAssetHandle?
        virtual void IncRef(Handle<Texture> textureHandle) = 0;
        virtual void DecRef(Handle<Texture> textureHandle) = 0;

        virtual void IncRef(Handle<FrameBuffer> textureHandle) = 0;
        virtual void DecRef(Handle<FrameBuffer> textureHandle) = 0;

        virtual void IncRef(Handle<IndexBuffer> textureHandle) = 0;
        virtual void DecRef(Handle<IndexBuffer> textureHandle) = 0;

        virtual void IncRef(Handle<VertexBuffer> textureHandle) = 0;
        virtual void DecRef(Handle<VertexBuffer> textureHandle) = 0;

        virtual void IncRef(Handle<GpuBuffer> textureHandle) = 0;
        virtual void DecRef(Handle<GpuBuffer> textureHandle) = 0;

        virtual void IncRef(Handle<RenderPass> textureHandle) = 0;
        virtual void DecRef(Handle<RenderPass> textureHandle) = 0;

        virtual void IncRef(Handle<ShaderResourceBinding> textureHandle) = 0;
        virtual void DecRef(Handle<ShaderResourceBinding> textureHandle) = 0;

        virtual void IncRef(Handle<PipelineStateObject> textureHandle) = 0;
        virtual void DecRef(Handle<PipelineStateObject> textureHandle) = 0;

        virtual bool IsAlive(PipelineStateHandle pipelineStateHandle) = 0;

        virtual void Shutdown() = 0;
    };
}
