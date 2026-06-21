#pragma once

#include "Quelos/Utility/SlotMap.h"

namespace Quelos {
    // Only shader types implemented for now are vertex and fragment
    enum class ShaderType : uint32_t {
        Unknown = 0x0000, ///< Unknown shader type
        Vertex = 0x0001, ///< Vertex shader  -- USED
        Fragment = 0x0002, ///< Fragment (pixel) shader -- USED
        Geometry = 0x0004, ///< Geometry shader
        Hull = 0x0008, ///< Hull (tessellation control) shader
        Domain = 0x0010, ///< Domain (tessellation evaluation) shader
        Compute = 0x0020, ///< Compute shader
        Amplification = 0x0040, ///< Amplification (task) shader
        Mesh = 0x0080, ///< Mesh shader
        RayGen = 0x0100, ///< Ray generation shader
        RayMiss = 0x0200, ///< Ray miss shader
        RayClosestHit = 0x0400, ///< Ray closest hit shader
        RayAnyHit = 0x0800, ///< Ray any hit shader
        RayIntersection = 0x1000, ///< Ray intersection shader
        Callable = 0x2000, ///< Callable shader
        Tile = 0x4000, ///< Tile shader (Only for Metal backend)
        Last = Tile,

        /// Vertex and fragment shader stages
        VertexAndFragment = Vertex | Fragment,

        /// All graphics pipeline shader stages
        AllGraphics = Vertex | Fragment | Geometry | Hull | Domain,

        /// All mesh shading pipeline stages
        AllMesh = Amplification | Mesh | Fragment,

        /// All ray-tracing pipeline shader stages
        ALlRayTracing = RayGen | RayMiss | RayClosestHit | RayAnyHit | RayIntersection | Callable,

        /// All shader stages
        SHADER_TYPE_ALL = Last * 2 - 1
    };

    enum class ShaderResourceVariableType : uint8_t {
        /// Shader resource bound to the variable is the same for all SRB instances.
        /// It must be set *once* directly through Pipeline State object.
        Static = 0,
        /// Shader resource bound to the variable is specific to the shader resource binding
            /// instance (see Diligent::IShaderResourceBinding). It must be set *once* through
            /// Diligent::IShaderResourceBinding interface. It cannot be set through Diligent::IPipelineState
            /// interface and cannot be change once bound.
        Mutable,

        /// Shader variable binding is dynamic. It can be set multiple times for every instance of shader resource
            /// binding (see Diligent::IShaderResourceBinding). It cannot be set through Diligent::IPipelineState interface.
        Dynamic,

        /// Total number of shader variable types
        Count
    };

    enum class ShaderResourceType : uint8_t {
        Unknown = 0,

        /// Constant (uniform) buffer
        ConstantBuffer,

        /// Shader resource view of a texture (sampled image)
        TextureSRV,

        /// Shader resource view of a buffer (read-only storage image)
        BufferSRV,

        /// Unordered access view of a texture (storage image)
        TextureUAV,

        /// Unordered access view of a buffer (storage buffer)
        BufferUAV,

        /// Sampler (separate sampler)
        Sampler,

        /// Input attachment in a render pass
        InputAttachment,

        /// Acceleration structure
        AccelStruct,

        Last = AccelStruct
    };

    enum class ShaderVariableFlags : uint8_t {
        /// Shader variable has no special properties.
        None = 0,

        /// Indicates that dynamic buffers will never be bound to the resource
        /// variable. Applies to Diligent::SHADER_RESOURCE_TYPE_CONSTANT_BUFFER,
        /// Diligent::SHADER_RESOURCE_TYPE_BUFFER_UAV, Diligent::SHADER_RESOURCE_TYPE_BUFFER_SRV resources.
        ///
        /// \remarks    This flag directly translates to the Diligent::PIPELINE_RESOURCE_FLAG_NO_DYNAMIC_BUFFERS
        ///             flag in the internal pipeline resource signature.
        NoDynamicBuffer = 1u << 0,

        /// Indicates that the resource consists of inline constants
        /// (also known as push constants in Vulkan or root constants in Direct3D12).
        /// Applies to Diligent::SHADER_RESOURCE_TYPE_CONSTANT_BUFFER only.
        ///
        /// \remarks    This flag directly translates to the Diligent::PIPELINE_RESOURCE_FLAG_INLINE_CONSTANTS
        ///             flag in the internal pipeline resource signature.
        ///
        /// See Diligent::PIPELINE_RESOURCE_FLAG_INLINE_CONSTANTS for more details.
        InlineConstants = 1u << 1,

        /// Indicates that the resource is an input attachment in general layout, which allows simultaneously
        /// reading from the resource through the input attachment and writing to it via color or depth-stencil
        /// attachment.
        ///
        /// \note This flag is only valid in Vulkan.
        GeneralInputAttachment_Vk = 1u << 2,

        /// Indicates that the resource is an unfilterable-float texture.
        ///
        /// \note This flag is only valid in WebGPU and ignored in other backends.
        UnfilterableFloatTexture_WebGPU = 1u << 3,

        /// Indicates that the resource is a non-filtering sampler.
        ///
        /// \note This flag is only valid in WebGPU and ignored in other backends.
        NonFilteringSampler_WebGPU = 1u << 4,

        /// Special value that indicates the last flag in the enumeration.
        Last = NonFilteringSampler_WebGPU
    };

    struct QS_API ShaderResourceVariableSpec {
        std::string_view Name;
        ShaderType ShaderStages = ShaderType::Unknown;
        ShaderResourceVariableType Type = ShaderResourceVariableType::Static;
        ShaderVariableFlags Flags = ShaderVariableFlags::None;
    };

    struct QS_API ShaderSpec {
        std::string_view Name;
        std::string_view EntryPoint;
        ShaderType Type;
    };

    struct QS_API ShaderCreateInfo {
        ShaderSpec Specification;
        BufferView ByteCode;
    };

    class Shader;

    struct QS_API ShaderHandle : Handle<Shader> {
        ShaderHandle() = default;
        ShaderHandle(const Handle shaderHandle) : Handle(shaderHandle) {}
    };
}
