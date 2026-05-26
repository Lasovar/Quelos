#pragma once

#include <cstdint>

namespace Quelos {
    enum class ComparisonFunc : uint8_t {
        /// Unknown comparison function
        Unknown = 0,

        /// Comparison never passes. \n
        /// Direct3D counterpart: D3D11_COMPARISON_NEVER/D3D12_COMPARISON_FUNC_NEVER. OpenGL counterpart: GL_NEVER.
        Never,

        /// Comparison passes if the source data is less than the destination data.\n
        /// Direct3D counterpart: D3D11_COMPARISON_LESS/D3D12_COMPARISON_FUNC_LESS. OpenGL counterpart: GL_LESS.
        Less,

        /// Comparison passes if the source data is equal to the destination data.\n
        /// Direct3D counterpart: D3D11_COMPARISON_EQUAL/D3D12_COMPARISON_FUNC_EQUAL. OpenGL counterpart: GL_EQUAL.
        Equal,

        /// Comparison passes if the source data is less than or equal to the destination data.\n
        /// Direct3D counterpart: D3D11_COMPARISON_LESS_EQUAL/D3D12_COMPARISON_FUNC_LESS_EQUAL. OpenGL counterpart: GL_LEQUAL.
        LessEqual,

        /// Comparison passes if the source data is greater than the destination data.\n
        /// Direct3D counterpart: 3D11_COMPARISON_GREATER/D3D12_COMPARISON_FUNC_GREATER. OpenGL counterpart: GL_GREATER.
        Greater,

        /// Comparison passes if the source data is not equal to the destination data.\n
        /// Direct3D counterpart: D3D11_COMPARISON_NOT_EQUAL/D3D12_COMPARISON_FUNC_NOT_EQUAL. OpenGL counterpart: GL_NOTEQUAL.
        NotEqual,

        /// Comparison passes if the source data is greater than or equal to the destination data.\n
        /// Direct3D counterpart: D3D11_COMPARISON_GREATER_EQUAL/D3D12_COMPARISON_FUNC_GREATER_EQUAL. OpenGL counterpart: GL_GEQUAL.
        GreaterEqual,

        /// Comparison always passes. \n
        /// Direct3D counterpart: D3D11_COMPARISON_ALWAYS/D3D12_COMPARISON_FUNC_ALWAYS. OpenGL counterpart: GL_ALWAYS.
        Always,

        /// Helper value that stores the total number of comparison functions in the enumeration
        Count
    };

    enum class Bind : uint32_t {
        /// Undefined binding.
        None = 0,

        /// A buffer can be bound as a vertex buffer.
        VertexBuffer = 1u << 0u,

        /// A buffer can be bound as an index buffer.
        IndexBuffer = 1u << 1u,

        /// A buffer can be bound as a uniform buffer.
        ///
        /// \warning This flag may not be combined with any other bind flag.
        UniformBuffer = 1u << 2u,

        /// A buffer or a texture can be bound as a shader resource.
        ShaderResource = 1u << 3u,

        /// A buffer can be bound as a target for stream output stage.
        StreamOutput = 1u << 4u,

        /// A texture can be bound as a render target.
        RenderTarget = 1u << 5u,

        /// A texture can be bound as a depth-stencil target.
        DepthStencil = 1u << 6u,

        /// A buffer or a texture can be bound as an unordered access view.
        UnorderedAccess = 1u << 7u,

        /// A buffer can be bound as the source buffer for indirect draw commands.
        IndirectDrawArgs = 1u << 8u,

        /// A texture can be used as render pass input attachment.
        InputAttachment = 1u << 9u,

        /// A buffer can be used as a scratch buffer or as the source of primitive data
        /// for acceleration structure building.
        RayTracing = 1u << 10u,

        ///< A texture can be used as shading rate texture.
        ShadingRate = 1u << 11u,

        Last = ShadingRate
    };

    using BindFlags = Bind;

    enum class CpuAccess : uint8_t {
        None = 0u, ///< No CPU access
        Read = 1u << 0u, ///< A resource can be mapped for reading
        Write = 1u << 1u, ///< A resource can be mapped for writing

        Last = Write
    };

    using CpuAccessFlags = CpuAccess;

    enum class Usage : uint8_t {
        /// A resource that can only be read by the GPU. It cannot be written by the GPU,
        /// and cannot be accessed at all by the CPU. This type of resource must be initialized
        /// when it is created, since it cannot be changed after creation. \n
        /// D3D11 Counterpart: D3D11_USAGE_IMMUTABLE. OpenGL counterpart: GL_STATIC_DRAW
        /// \remarks Static buffers do not allow CPU access and must use CpuAccess::None flag.
        Immutable = 0,

        /// A resource that requires read and write access by the GPU and can also be occasionally
        /// written by the CPU.  \n
        /// D3D11 Counterpart: D3D11_USAGE_DEFAULT. OpenGL counterpart: GL_STREAM_DRAW.
        /// \remarks Default buffers do not allow CPU access and must use CpuAccess::None flag.
        Default,

        /// A resource that can be read by the GPU and written at least once per frame by the CPU.  \n
        /// D3D11 Counterpart: D3D11_USAGE_DYNAMIC. OpenGL counterpart: GL_DYNAMIC_DRAW
        /// \remarks Dynamic buffers must use CpuAccess::Write flag.
        Dynamic,

        /// A resource that facilitates transferring data between GPU and CPU. \n
        /// D3D11 Counterpart: D3D11_USAGE_STAGING. OpenGL counterpart: GL_STREAM_READ or
        /// GL_STREAM_DRAW depending on the CPU access flags.
        /// \remarks Staging buffers must use exactly one of CpuAccess::Write or CpuAccess::Read flags.
        Staging,

        /// A resource residing in a unified memory (e.g. memory shared between CPU and GPU),
        /// that can be read and written by GPU and can also be directly accessed by CPU.
        ///
        /// An application should check if unified memory is available on the device by querying
        /// the adapter info (see Diligent::IRenderDevice::GetAdapterInfo().Memory and Diligent::AdapterMemoryInfo).
        /// If there is no unified memory, an application should choose another usage type (typically, USAGE_DEFAULT).
        ///
        /// Unified resources must use at least one of CpuAccess::Write or CpuAccess::Read flags.
        /// An application should check supported unified memory CPU access types by querying the device caps.
        /// (see Diligent::AdapterMemoryInfo::UnifiedMemoryCPUAccess).
        Unified,

        /// A resource that can be partially committed to physical memory.
        Sparse,

        /// Helper value indicating the total number of elements in the enum
        Count
    };


    enum class Map : uint8_t {
        /// The resource is mapped for reading. \n
        /// D3D11 counterpart: D3D11_MAP_READ. OpenGL counterpart: GL_MAP_READ_BIT
        Read = 0x01,

        /// The resource is mapped for writing. \n
            /// D3D11 counterpart: D3D11_MAP_WRITE. OpenGL counterpart: GL_MAP_WRITE_BIT
        Write = 0x02,

        /// The resource is mapped for reading and writing. \n
            /// D3D11 counterpart: D3D11_MAP_READ_WRITE. OpenGL counterpart: GL_MAP_WRITE_BIT | GL_MAP_READ_BIT
        ReadWrite = 0x03
    };

    using MapType = Map;

    enum class MapFlags : uint8_t {
        /// No special flags
        None = 0x000,

        /// Specifies that map operation should not wait until previous command that
        /// using the same resource completes. Map returns null pointer if the resource
        /// is still in use.\n
        /// D3D11 counterpart:  D3D11_MAP_FLAG_DO_NOT_WAIT
        /// \note OpenGL does not have corresponding flag, so a buffer will always be mapped
        DoNotWait = 0x001,

        /// Previous contents of the resource will be undefined. This flag is only compatible with MAP_WRITE\n
        /// D3D11 counterpart: D3D11_MAP_WRITE_DISCARD. OpenGL counterpart: GL_MAP_INVALIDATE_BUFFER_BIT
        /// \note OpenGL implementation may orphan a buffer instead
        Discard = 0x002,

        /// The system will not synchronize pending operations before mapping the buffer. It is responsibility
        /// of the application to make sure that the buffer contents is not overwritten while it is in use by
        /// the GPU.\n
        /// D3D11 counterpart:  D3D11_MAP_WRITE_NO_OVERWRITE. OpenGL counterpart: GL_MAP_UNSYNCHRONIZED_BIT
        NoOverwrite = 0x004
    };
}
