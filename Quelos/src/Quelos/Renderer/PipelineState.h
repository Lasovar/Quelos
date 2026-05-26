#pragma once

#include "Color.h"
#include "DepthStencilState.h"
#include "GraphicsTypes.h"
#include "LayoutElement.h"
#include "RasterizerState.h"
#include "RenderPass.h"
#include "Shader.h"
#include "Texture.h"
#include "Quelos/Core/Base.h"

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
        TextureFilter MinFilter = TextureFilter::Linear;
        TextureFilter MagFilter = TextureFilter::Linear;
        TextureFilter MipFilter = TextureFilter::Linear;
        TextureWrap WrapU = TextureWrap::Clamp;
        TextureWrap WrapV = TextureWrap::Clamp;
        TextureWrap WrapW = TextureWrap::Clamp;
        SamplerFlags Flags;

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

    struct PipelineResourceLayoutSpec {
        ShaderResourceVariableType DefaultVariableType;
        Span32<const ShaderResourceVariableSpec> Variables;
        Span32<const ImmutableSamplerSpec> ImmutableSamplers;
    };

    enum class PipelineType {
        /// Graphics pipeline, which is used by IDeviceContext::Draw(), IDeviceContext::DrawIndexed(),
        /// IDeviceContext::DrawIndirect(), IDeviceContext::DrawIndexedIndirect().
        Graphics, // ONLY ONE USED RN
        /// Compute pipeline, which is used by IDeviceContext::DispatchCompute(), IDeviceContext::DispatchComputeIndirect().
        Compute,
        /// Mesh pipeline, which is used by IDeviceContext::DrawMesh(), IDeviceContext::DrawMeshIndirect().
        Mesh,
        /// Ray tracing pipeline, which is used by IDeviceContext::TraceRays().
        RayTracing,
        /// Tile pipeline, which is used by IDeviceContext::DispatchTile().
        Tile,
        /// Special value that indicates the last pipeline type in the enumeration.
        Last = Tile,
        /// Number of pipeline types in the enumeration.
        Count,
        /// Invalid pipeline type.
        Invalid = 0xFF
    };

    struct PipelineStateSpec {
        PipelineType Type = PipelineType::Graphics;
        PipelineResourceLayoutSpec ResourceLayout;
    };

    enum class PsoCreateFlags : uint32_t {
        /// Null flag.
        None = 0u,

        /// Ignore missing variables.

        /// By default, the engine outputs a warning for every variable
        /// provided as part of the pipeline resource layout description
        /// that is not found in any of the designated shader stages.
        /// Use this flag to silence these warnings.
        IgnoreMissingVariables = 1u << 0u,

        /// Ignore missing immutable samplers.

        /// By default, the engine outputs a warning for every immutable sampler
        /// provided as part of the pipeline resource layout description
        /// that is not found in any of the designated shader stages.
        /// Use this flag to silence these warnings.
        IgnoreMissingImmutableSamplers = 1u << 1u,

        /// Do not remap shader resources when creating the pipeline.

        /// Resource bindings in all shaders must match the bindings expected
        /// by the PSO's resource signatures.
        DontRemapShaderResources = 1u << 2u,

        /// Create the pipeline state asynchronously.

        /// When this flag is set to true and if the devices supports
        /// AsyncShaderCompilation feature, the pipeline will be created
        /// asynchronously in the background. An application should use
        /// the IPipelineState::GetStatus() method to check the pipeline status.
        /// If the device does not support asynchronous shader compilation,
        /// the flag is ignored and the pipeline is created synchronously.
        Asynchronous = 1u << 3u,

        Last = Asynchronous
    };

    struct PipelineStateCreateInfo {
        PipelineStateSpec Spec;
        PsoCreateFlags Flags = PsoCreateFlags::None;
    };

    struct BlendState {
        /// Specifies whether to use alpha-to-coverage as a multisampling technique
        /// when setting a pixel to a render target. Default value: False.
        bool AlphaToCoverageEnable = false;


        /// Specifies whether to enable independent blending in simultaneous render targets.
        /// If set to False, only RenderTargets[0] is used. Default value: False.
        bool IndependentBlendEnable = false;

        /// An array of RenderTargetBlendDesc structures that describe the blend
        /// states for render targets
        //RenderTargetBlendDesc RenderTargets[DILIGENT_MAX_RENDER_TARGETS]; // TODO:
    };

    struct GraphicsPipelineSpec {
        BlendState BlendState;

        /// Sample mask.

        /// 32-bit sample mask that determines which samples get updated
        /// in all the active render targets. A sample mask is always applied;
        /// it is independent of whether multisampling is enabled, and does not
        /// depend on whether an application uses multisample render targets.
        uint32_t SampleMask = 0xFFFFFFFF;
        RasterizerStateSpec RasterizerSpec;
        DepthStencilStateSpec DepthStencilSpec;
        InputLayoutSpec InputLayout;
        RenderPassHandle RenderPass;
    };

    struct GraphicsPipelineStateCreateInfo : PipelineStateCreateInfo {
        std::string_view Name;
        GraphicsPipelineSpec GraphicsPipeline;
        ShaderHandle VertexShader;
        ShaderHandle FragmentShader;
        // TODO: Add other shader stages
    };

    class PipelineStateObject;

    struct PipelineStateHandle : Handle<PipelineStateObject> {
        PipelineStateHandle() = default;
        PipelineStateHandle(Handle handle) : Handle(handle) {}
    };
}
