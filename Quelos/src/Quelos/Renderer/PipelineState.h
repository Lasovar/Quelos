#pragma once

#include "BlendState.h"
#include "DepthStencilState.h"
#include "GraphicsTypes.h"
#include "LayoutElement.h"
#include "PipelineResourceSignature.h"
#include "RasterizerState.h"
#include "RenderPass.h"
#include "Shader.h"
#include "Quelos/Core/Base.h"

namespace Quelos {

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

    struct SampleSpec {
        /// Sample count
        SampleCount Count = SampleCount::x1;

        /// Quality
        uint8_t Quality = 0;
    };

    struct GraphicsPipelineSpec {
        BlendStateSpec BlendSpec;

        /// Sample mask.

        /// 32-bit sample mask that determines which samples get updated
        /// in all the active render targets. A sample mask is always applied;
        /// it is independent of whether multisampling is enabled, and does not
        /// depend on whether an application uses multisample render targets.
        uint32_t SampleMask = 0xFFFFFFFF;
        RasterizerStateSpec RasterizerSpec;
        DepthStencilStateSpec DepthStencilSpec;
        InputLayoutSpec InputLayout;

        /// The number of render targets in the RTVFormats array.

        /// Must be 0 when `RenderPass` is not `Invalid`.
        uint32_t NumRenderTargets = 0;

        RenderPassHandle RenderPass;

        SampleSpec SampleSpec;
    };

    struct GraphicsPipelineStateCreateInfo : PipelineStateCreateInfo {
        std::string_view Name;
        GraphicsPipelineSpec GraphicsPipeline;
        ShaderHandle VertexShader;
        ShaderHandle FragmentShader;
        // TODO: Add other shader stages
    };

    struct ComputePipelineStateCreateInfo : PipelineStateCreateInfo {
        ShaderHandle ComputeShader;
    };

    class PipelineStateObject;

    struct PipelineStateHandle : Handle<PipelineStateObject> {
        PipelineStateHandle() = default;
        PipelineStateHandle(Handle handle) : Handle(handle) {}
    };
}
