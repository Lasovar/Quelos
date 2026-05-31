//
// Created by lasovar on 5/31/26.
//

#pragma once

#include <string_view>

#include "Color.h"
#include "Shader.h"

namespace Quelos {

    enum class SamplerFlags : uint8_t {
        /// No flags are set.
        None = 0,

        /// Specifies that the sampler will read from a subsampled texture created with MISC_TEXTURE_FLAG_SUBSAMPLED flag.
        /// Requires SHADING_RATE_CAP_FLAG_SUBSAMPLED_RENDER_TARGET capability.
        Subsampled = 1u << 0,

        /// Specifies that the GPU is allowed to use fast approximation when reconstructing full-resolution value from
        /// the subsampled texture accessed by the sampler.
        /// Requires SHADING_RATE_CAP_FLAG_SUBSAMPLED_RENDER_TARGET capability.
        SubsampledCoarseReconstruction = 1u << 1,
        Last = SubsampledCoarseReconstruction
    };

    struct SamplerSpec {
        FilterMode MinFilter = FilterMode::Linear;
        FilterMode MagFilter = FilterMode::Linear;
        FilterMode MipFilter = FilterMode::Linear;
        WrapMode WrapU = WrapMode::Clamp;
        WrapMode WrapV = WrapMode::Clamp;
        WrapMode WrapW = WrapMode::Clamp;
        SamplerFlags Flags = SamplerFlags::None;

        /// Indicates whether to use unnormalized texture coordinates.

        /// When set to `True`, the range of the image coordinates used to lookup
        /// the texel is in the range of 0 to the image size in each dimension.
        /// When set to `False`, the range of image coordinates is 0.0 to 1.0.
        ///
        /// Unnormalized coordinates are only supported in Vulkan and Metal.
        bool UnnormalizedCoords = false;

        /// Offset from the calculated mipmap level.

        /// For example, if a sampler calculates that a texture should be sampled at mipmap
        /// level 1.2 and MipLODBias is 2.3, then the texture will be sampled at
        /// mipmap level 3.5.
        ///
        /// Default value: 0.
        float MipLODBias = 0.0f;

        /// Maximum anisotropy level for the anisotropic filter. Default value: 0.
        uint32_t MaxAnisotropy = 0;

        /// A function that compares sampled data against existing sampled data when comparison filter is used.

        /// Default value: Diligent::COMPARISON_FUNC_NEVER.
        ComparisonFunc ComparisonFunc = ComparisonFunc::Never;

        /// Border color to use if TEXTURE_ADDRESS_BORDER is specified for `AddressU`, `AddressV`, or `AddressW`.

        /// Default value: `{0, 0, 0, 0}`
        Color BorderColor = Color::Black();

        /// Specifies the minimum value that LOD is clamped to before accessing the texture MIP levels.

        /// Must be less than or equal to `MaxLOD`.
        ///
        /// Default value: 0.
        float MinLOD = 0;

        /// Specifies the maximum value that LOD is clamped to before accessing the texture MIP levels.

        /// Must be greater than or equal to `MinLOD`.
        /// Default value: `+FLT_MAX`.
        float MaxLOD = +3.402823466e+38F;
    };

    struct ImmutableSamplerSpec {
        ShaderType ShaderStages = ShaderType::Unknown;
        std::string_view SamplerOrTextureName;
        SamplerSpec Specification;
    };

    enum class PipelineResourceFlag : uint8_t {
        /// Resource has no special properties
        None = 0,

        /// Indicates that dynamic buffers will never be bound to the resource
        /// variable. Applies to SHADER_RESOURCE_TYPE_CONSTANT_BUFFER,
        /// SHADER_RESOURCE_TYPE_BUFFER_UAV, SHADER_RESOURCE_TYPE_BUFFER_SRV resources.
        ///
        /// In Vulkan and Direct3D12 backends, dynamic buffers require extra work
        /// at run time. If an application knows it will never bind a dynamic buffer to
        /// the variable, it should use PIPELINE_RESOURCE_FLAG_NO_DYNAMIC_BUFFERS flag
        /// to improve performance. This flag is not required and non-dynamic buffers
        /// will still work even if the flag is not used. It is an error to bind a
        /// dynamic buffer to resource that uses
        /// PIPELINE_RESOURCE_FLAG_NO_DYNAMIC_BUFFERS flag.
        NoDynamicBuffers = 1u << 0,

        /// Indicates that the resource consists of inline constants (also
        /// known as push constants in Vulkan or root constants in Direct3D12).
        ///
        /// Applies to SHADER_RESOURCE_TYPE_CONSTANT_BUFFER only.
        ///
        /// Use this flag if you have a buffer of frequently changing constants
        /// - that are small in size (typically up to 128 bytes) and
        /// - change often (e.g. per-draw or per-dispatch).
        ///
        /// Inline constants are set directly using IShaderResourceVariable::SetInlineConstants.
        ///
        /// This flag cannot be combined with any other flags.
        ///
        /// In Vulkan and Direct3D12, inline constants are not bound via descriptor sets or root
        /// signatures, but are set directly in command buffers or command lists and are very cheap.
        /// In legacy APIs (Direct3D11 and OpenGL), inline constants are emulated using regular
        /// constant buffers and thus have higher overhead.
        InlineConstants = 1u << 1,

        /// Indicates that a texture SRV will be combined with a sampler.
        /// Applies to SHADER_RESOURCE_TYPE_TEXTURE_SRV resources.
        CombinedSampler = 1u << 2,

        /// Indicates that this variable will be used to bind formatted buffers.
        /// Applies to SHADER_RESOURCE_TYPE_BUFFER_UAV and SHADER_RESOURCE_TYPE_BUFFER_SRV
        /// resources.
        ///
        /// In Vulkan backend formatted buffers require another descriptor type
        /// as opposed to structured buffers. If an application will be using
        /// formatted buffers with buffer UAVs and SRVs, it must specify the
        /// PIPELINE_RESOURCE_FLAG_FORMATTED_BUFFER flag.
        FormattedBuffer = 1u << 3,

        /// Indicates that resource is a run-time sized shader array (e.g. an array without a specific size).
        RuntimeArray = 1u << 4,

        /// Indicates that the resource is an input attachment in general layout, which allows simultaneously
        /// reading from the resource through the input attachment and writing to it via color or depth-stencil
        /// attachment.
        ///
        /// \note This flag is only valid in Vulkan.
        GeneralInputAttachment = 1u << 5,

        Last = GeneralInputAttachment
    };

    struct PipelineResourceSpec {
        std::string_view Name;
        ShaderType ShaderStages = ShaderType::Unknown;

        /// Resource array size (must be 1 for non-array resources).
        ///
        /// For inline constants (see PipelineResourceFlag::InlineConstants),
        /// this member specifies the number of 4-byte values.
        uint32_t ArraySize = 1;
        ShaderResourceType ResourceType = ShaderResourceType::Unknown;
        ShaderResourceVariableType VariableType = ShaderResourceVariableType::Mutable;
        PipelineResourceFlag Flags = PipelineResourceFlag::None;
    };

    struct PipelineResourceSignatureSpec {
        std::string_view Name;
        Span32<const PipelineResourceSpec> Resources;
        Span32<const ImmutableSamplerSpec> ImmutableSamplers;
        uint8_t BindingIndex = 0;
        bool UseCombinedTextureSamplers = false;
        std::string_view CombinedSamplerSuffix = "_sampler";

        /// Shader resource binding allocation granularity

        /// This member defines the allocation granularity for internal resources required by
        /// the shader resource binding object instances.
        uint32_t SRBAllocationGranularity = 1;
    };

    class PipelineResourceSignature;

    struct PipelineResourceSignatureHandle : Handle<PipelineResourceSignature> {
        PipelineResourceSignatureHandle() = default;
        PipelineResourceSignatureHandle(const Handle handle) : Handle(handle) {}
    };
}
