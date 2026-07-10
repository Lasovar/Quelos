#include "DiligentRendererContext.h"

#include "DataBlobImpl.hpp"
#include "Texture.h"

#include "ImGui/MapHelper.hpp"
#include "Quelos/ImGui/ImGuiUI.h"

#if QS_PLATFORM_MACOS
#include "Quelos/Platform/MacOS/WindowHelper.h"
#endif

#include "magic_enum/magic_enum.hpp"
using namespace magic_enum::bitwise_operators;

namespace Quelos {
    namespace Utils {
        static void CreateFrameBuffer(
            IFramebuffer*& frameBuffer, IRenderDevice* device, const char* name,
            const Span32<ITextureView*> attachments, const uint32_t numArraySlices, IRenderPass* renderPass
        ) {
            FramebufferDesc desc;
            desc.Name = name;
            desc.ppAttachments = attachments.data();
            desc.AttachmentCount = attachments.size();
            if (renderPass) {
                desc.pRenderPass = renderPass;
            }
            desc.Width = attachments[0]->GetTexture()->GetDesc().GetWidth();
            desc.Height = attachments[0]->GetTexture()->GetDesc().GetHeight();
            desc.NumArraySlices = numArraySlices;

            device->CreateFramebuffer(desc, &frameBuffer);
        }

        static TEXTURE_FORMAT GetFormat(const ImageFormat imageFormat) {
            switch (imageFormat) {
            case ImageFormat::None:
                return TEX_FORMAT_UNKNOWN;
            case ImageFormat::R8UNorm:
                return TEX_FORMAT_R8_UNORM;
            case ImageFormat::R8UInt:
                return TEX_FORMAT_R8_UINT;
            case ImageFormat::R16UInt:
                return TEX_FORMAT_R16_UINT;
            case ImageFormat::R32UInt:
                return TEX_FORMAT_R32_UINT;
            case ImageFormat::R32Float:
                return TEX_FORMAT_R32_FLOAT;
            case ImageFormat::RG8UNorm:
                return TEX_FORMAT_RG8_UNORM;
            case ImageFormat::RG16Float:
                return TEX_FORMAT_RG16_FLOAT;
            case ImageFormat::RG32Float:
                return TEX_FORMAT_RG32_FLOAT;
            case ImageFormat::RGB:
                return TEX_FORMAT_ETC2_RGB8_UNORM;
            case ImageFormat::RGBA8UNorm:
                return TEX_FORMAT_RGBA8_UNORM;
            case ImageFormat::RGBA8SNorm:
                return TEX_FORMAT_RGBA8_SNORM;
            case ImageFormat::RGBA16Float:
                return TEX_FORMAT_RGBA16_FLOAT;
            case ImageFormat::RGBA32Float:
                return TEX_FORMAT_RGBA32_FLOAT;
            case ImageFormat::B10R11G11Float:
                return TEX_FORMAT_R11G11B10_FLOAT;
            case ImageFormat::SRGB:
                return TEX_FORMAT_BC7_UNORM_SRGB;
            case ImageFormat::SRGBA:
                return TEX_FORMAT_RGBA8_UNORM_SRGB;
            case ImageFormat::Depth32FloatStencil8UInt:
                return TEX_FORMAT_D32_FLOAT_S8X24_UINT;
            case ImageFormat::Depth32Float:
                return TEX_FORMAT_D32_FLOAT;
            case ImageFormat::Depth24Stencil8:
                return TEX_FORMAT_D24_UNORM_S8_UINT;
            }

            return TEX_FORMAT_UNKNOWN;
        }

        constexpr RESOURCE_STATE GetResourceState(const ResourceState state) {
            RESOURCE_STATE result = RESOURCE_STATE_UNKNOWN;

            if ((state & ResourceState::VertexBuffer) != ResourceState::Unknown) {
                result |= RESOURCE_STATE_VERTEX_BUFFER;
            }
            if ((state & ResourceState::ConstantBuffer) != ResourceState::Unknown) {
                result |= RESOURCE_STATE_CONSTANT_BUFFER;
            }
            if ((state & ResourceState::IndexBuffer) != ResourceState::Unknown) {
                result |= RESOURCE_STATE_INDEX_BUFFER;
            }
            if ((state & ResourceState::RenderTarget) != ResourceState::Unknown) {
                result |= RESOURCE_STATE_RENDER_TARGET;
            }
            if ((state & ResourceState::UnorderedAccess) != ResourceState::Unknown) {
                result |= RESOURCE_STATE_UNORDERED_ACCESS;
            }
            if ((state & ResourceState::DepthWrite) != ResourceState::Unknown) {
                result |= RESOURCE_STATE_DEPTH_WRITE;
            }
            if ((state & ResourceState::DepthRead) != ResourceState::Unknown) {
                result |= RESOURCE_STATE_DEPTH_READ;
            }
            if ((state & ResourceState::ShaderResource) != ResourceState::Unknown) {
                result |= RESOURCE_STATE_SHADER_RESOURCE;
            }
            if ((state & ResourceState::IndirectArgument) != ResourceState::Unknown) {
                result |= RESOURCE_STATE_INDIRECT_ARGUMENT;
            }
            if ((state & ResourceState::CopySource) != ResourceState::Unknown) {
                result |= RESOURCE_STATE_COPY_SOURCE;
            }
            if ((state & ResourceState::CopyDest) != ResourceState::Unknown) {
                result |= RESOURCE_STATE_COPY_DEST;
            }
            if ((state & ResourceState::ResolveSource) != ResourceState::Unknown) {
                result |= RESOURCE_STATE_RESOLVE_SOURCE;
            }
            if ((state & ResourceState::ResolveDest) != ResourceState::Unknown) {
                result |= RESOURCE_STATE_RESOLVE_DEST;
            }
            if ((state & ResourceState::InputAttachment) != ResourceState::Unknown) {
                result |= RESOURCE_STATE_INPUT_ATTACHMENT;
            }
            if ((state & ResourceState::Present) != ResourceState::Unknown) {
                result |= RESOURCE_STATE_PRESENT;
            }
            if ((state & ResourceState::BuildAsRead) != ResourceState::Unknown) {
                result |= RESOURCE_STATE_BUILD_AS_READ;
            }
            if ((state & ResourceState::BuildAsWrite) != ResourceState::Unknown) {
                result |= RESOURCE_STATE_BUILD_AS_WRITE;
            }
            if ((state & ResourceState::RayTracing) != ResourceState::Unknown) {
                result |= RESOURCE_STATE_RAY_TRACING;
            }
            if ((state & ResourceState::ShadingRate) != ResourceState::Unknown) {
                result |= RESOURCE_STATE_SHADING_RATE;
            }

            return result;
        }

        static ATTACHMENT_LOAD_OP GetAttachmentLoadOp(const AttachmentLoadOp loadOp) {
            switch (loadOp) {
            case AttachmentLoadOp::Load: return ATTACHMENT_LOAD_OP_LOAD;
            case AttachmentLoadOp::Clear: return ATTACHMENT_LOAD_OP_CLEAR;
            case AttachmentLoadOp::Discard: return ATTACHMENT_LOAD_OP_DISCARD;
            }

            return ATTACHMENT_LOAD_OP_LOAD;
        }

        static ATTACHMENT_STORE_OP GetAttachmentStoreOp(const AttachmentStoreOp storeOp) {
            switch (storeOp) {
            case AttachmentStoreOp::Store: return ATTACHMENT_STORE_OP_STORE;
            case AttachmentStoreOp::Discard: return ATTACHMENT_STORE_OP_DISCARD;
            }

            return ATTACHMENT_STORE_OP_STORE;
        }

        static RenderPassAttachmentDesc GetRenderPassAttachmentDesc(const RenderPassAttachmentSpec& spec) {
            RenderPassAttachmentDesc desc;
            desc.InitialState = GetResourceState(spec.InitialState);
            desc.FinalState = GetResourceState(spec.FinalState);
            desc.LoadOp = GetAttachmentLoadOp(spec.LoadOp);
            desc.StoreOp = GetAttachmentStoreOp(spec.StoreOp);
            desc.SampleCount = spec.SampleCount;
            desc.Format = GetFormat(spec.Format);
            desc.StencilLoadOp = GetAttachmentLoadOp(spec.StencilLoadOp);
            desc.StencilStoreOp = GetAttachmentStoreOp(spec.StencilStoreOp);

            return desc;
        }

        static Diligent::AttachmentReference GetAttachmentReference(const AttachmentReference& attachmentReference) {
            Diligent::AttachmentReference result;
            result.AttachmentIndex = attachmentReference.AttachmentIndex;
            result.State = GetResourceState(attachmentReference.State);
            return result;
        }

        constexpr SHADER_TYPE GetShaderType(const ShaderType type) {
            return static_cast<SHADER_TYPE>(type);
        }

        constexpr ShaderType FromDiligentShaderType(const SHADER_TYPE type) {
            return static_cast<ShaderType>(type);
        }

        constexpr SHADER_RESOURCE_VARIABLE_TYPE GetShaderResourceVariableType(const ShaderResourceVariableType type) {
            switch (type) {
            case ShaderResourceVariableType::Static:
                return SHADER_RESOURCE_VARIABLE_TYPE_STATIC;

            case ShaderResourceVariableType::Mutable:
                return SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE;

            case ShaderResourceVariableType::Dynamic:
                return SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC;

            case ShaderResourceVariableType::Count:
                break;
            }

            return SHADER_RESOURCE_VARIABLE_TYPE_STATIC;
        }

        constexpr ShaderResourceVariableType FromDiligentShaderResourceVariableType(
            const SHADER_RESOURCE_VARIABLE_TYPE type
        ) {
            switch (type) {
            case SHADER_RESOURCE_VARIABLE_TYPE_STATIC: return ShaderResourceVariableType::Static;
            case SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE: return ShaderResourceVariableType::Mutable;
            case SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC: return ShaderResourceVariableType::Dynamic;
            default:
                return ShaderResourceVariableType::Static;
            }

            return ShaderResourceVariableType::Static;
        }

        constexpr SHADER_VARIABLE_FLAGS GetShaderVariableFlags(const ShaderVariableFlags flags) {
            return static_cast<SHADER_VARIABLE_FLAGS>(
                static_cast<std::underlying_type_t<ShaderVariableFlags>>(flags)
            );
        }

        constexpr ShaderVariableFlags FromDiligentShaderVariableFlags(const SHADER_VARIABLE_FLAGS flags) {
            return static_cast<ShaderVariableFlags>(
                static_cast<std::underlying_type_t<SHADER_VARIABLE_FLAGS>>(flags)
            );
        }

        constexpr CULL_MODE GetCullMode(const CullMode mode) {
            QS_CORE_ASSERT(mode != CullMode::Count);
            return static_cast<CULL_MODE>(mode);
        }

        constexpr INPUT_ELEMENT_FREQUENCY
        ToDiligentInputElementFrequency(
            const InputElementFrequency frequency
        ) {
            switch (frequency) {
            case InputElementFrequency::Undefined:
                return INPUT_ELEMENT_FREQUENCY_UNDEFINED;

            case InputElementFrequency::PerVertex:
                return INPUT_ELEMENT_FREQUENCY_PER_VERTEX;

            case InputElementFrequency::PerInstance:
                return INPUT_ELEMENT_FREQUENCY_PER_INSTANCE;

            case InputElementFrequency::Count:
                break;
            }

            return INPUT_ELEMENT_FREQUENCY_UNDEFINED;
        }

        constexpr VALUE_TYPE
        GetValueType(
            const ValueType type
        ) {
            switch (type) {
            case ValueType::Undefined:
                return VT_UNDEFINED;
            case ValueType::Float:
            case ValueType::Float2:
            case ValueType::Float3:
            case ValueType::Float4:
            case ValueType::Float3x3:
            case ValueType::Float4x4:
                return VT_FLOAT32;

            case ValueType::Int:
            case ValueType::Int2:
            case ValueType::Int3:
            case ValueType::Int4:
                return VT_INT32;

            case ValueType::UInt:
                return VT_UINT32;
            case ValueType::UInt16:
                return VT_UINT16;

            case ValueType::UInt10x3_A2:
                QS_ASSERT(false && "UInt10x3_A2 is not supported by Diligent VALUE_TYPE");
                return VT_UINT32;

            case ValueType::UNorm8x2:
            case ValueType::UNorm8x4:
                return VT_UINT8;

            case ValueType::UNorm16x2:
            case ValueType::UNorm16x4:
                return VT_UINT16;

            case ValueType::SNorm8x2:
            case ValueType::SNorm8x4:
                return VT_INT8;

            case ValueType::SNorm16x2:
            case ValueType::SNorm16x4:
                return VT_INT16;
            }

            return VT_FLOAT32;
        }

        struct DiligentInputLayoutDesc {
            Vec<Diligent::LayoutElement> Elements;
            InputLayoutDesc Desc;

            explicit DiligentInputLayoutDesc(const InputLayoutSpec& spec) {
                Elements.reserve(spec.LayoutElements.size());

                for (const auto& element : spec.LayoutElements) {
                    Elements.emplace_back(
                            element.InputIndex,
                            element.BufferSlot,
                            ComponentCount(element.Type),
                            GetValueType(element.Type),
                            element.IsNormalized,
                            element.RelativeOffset,
                            element.Stride,
                            ToDiligentInputElementFrequency(element.Frequency),
                            element.InstanceDataStepRate
                    );
                }

                Desc.LayoutElements = Elements.data();
                Desc.NumElements = spec.LayoutElements.size();
            }

            operator const Diligent::InputLayoutDesc&() const {
                return Desc;
            }
        };

        constexpr FILTER_TYPE GetFilterType(const FilterMode filter) {
            switch (filter) {
            case FilterMode::Point:
                return FILTER_TYPE_POINT;

            case FilterMode::Linear:
                return FILTER_TYPE_LINEAR;

            case FilterMode::Anisotropic:
                return FILTER_TYPE_ANISOTROPIC;

            case FilterMode::ComparisonPoint: return FILTER_TYPE_COMPARISON_POINT;
            case FilterMode::ComparisonLinear: return FILTER_TYPE_COMPARISON_LINEAR;
            case FilterMode::ComparisonAnisotropic: return FILTER_TYPE_COMPARISON_ANISOTROPIC;
            default: ;
            }

            return FILTER_TYPE_LINEAR;
        }

        constexpr TEXTURE_ADDRESS_MODE GetTextureAddressMode(const WrapMode wrap) {
            switch (wrap) {
            case WrapMode::Repeat:
                return TEXTURE_ADDRESS_WRAP;

            case WrapMode::Mirror:
                return TEXTURE_ADDRESS_MIRROR;

            case WrapMode::Clamp:
                return TEXTURE_ADDRESS_CLAMP;

            case WrapMode::Border:
                return TEXTURE_ADDRESS_BORDER;

            case WrapMode::MirrorOnce:
                return TEXTURE_ADDRESS_MIRROR_ONCE;
            default: ;
            }

            return TEXTURE_ADDRESS_CLAMP;
        }

        constexpr COMPARISON_FUNCTION GetComparisonFunc(const ComparisonFunc func) {
            switch (func) {
            case ComparisonFunc::Never:
                return COMPARISON_FUNC_NEVER;

            case ComparisonFunc::Less:
                return COMPARISON_FUNC_LESS;

            case ComparisonFunc::Equal:
                return COMPARISON_FUNC_EQUAL;

            case ComparisonFunc::LessEqual:
                return COMPARISON_FUNC_LESS_EQUAL;

            case ComparisonFunc::Greater:
                return COMPARISON_FUNC_GREATER;

            case ComparisonFunc::NotEqual:
                return COMPARISON_FUNC_NOT_EQUAL;

            case ComparisonFunc::GreaterEqual:
                return COMPARISON_FUNC_GREATER_EQUAL;

            case ComparisonFunc::Always:
                return COMPARISON_FUNC_ALWAYS;
            default: ;
            }

            return COMPARISON_FUNC_NEVER;
        }

        constexpr SAMPLER_FLAGS GetSamplerFlags(const SamplerFlags flags) {
            return static_cast<SAMPLER_FLAGS>(flags);
        }

        static SamplerDesc GetSamplerDesc(const SamplerSpec& spec) {
            SamplerDesc desc;

            desc.MinFilter = GetFilterType(spec.MinFilter);
            desc.MagFilter = GetFilterType(spec.MagFilter);
            desc.MipFilter = GetFilterType(spec.MipFilter);

            desc.AddressU = GetTextureAddressMode(spec.WrapU);
            desc.AddressV = GetTextureAddressMode(spec.WrapV);
            desc.AddressW = GetTextureAddressMode(spec.WrapW);

            desc.Flags = GetSamplerFlags(spec.Flags);

            desc.UnnormalizedCoords = spec.UnnormalizedCoords;

            desc.MipLODBias = spec.MipLODBias;
            desc.MaxAnisotropy = spec.MaxAnisotropy;

            desc.ComparisonFunc = GetComparisonFunc(spec.ComparisonFunc);

            desc.BorderColor[0] = spec.BorderColor.r;
            desc.BorderColor[1] = spec.BorderColor.g;
            desc.BorderColor[2] = spec.BorderColor.b;
            desc.BorderColor[3] = spec.BorderColor.a;

            desc.MinLOD = spec.MinLOD;
            desc.MaxLOD = spec.MaxLOD;

            return desc;
        }

        constexpr BIND_FLAGS GetBindFlags(BindFlags bindFlags) {
            return static_cast<BIND_FLAGS>(bindFlags);
        }

        constexpr CPU_ACCESS_FLAGS GetCPUAccessFlags(CpuAccessFlags cpuAccess) {
            return static_cast<CPU_ACCESS_FLAGS>(cpuAccess);
        }

        constexpr USAGE GetUsage(Usage usage) {
            return static_cast<USAGE>(usage);
        }

        constexpr MISC_BUFFER_FLAGS GetMiscFlags(MiscGpuBufferFlags miscFlags) {
            return static_cast<MISC_BUFFER_FLAGS>(miscFlags);
        }

        constexpr DRAW_FLAGS GetDrawFlags(DrawFlags flags) {
            return static_cast<DRAW_FLAGS>(flags);
        }

        constexpr MAP_TYPE GetMapType(MapType map) {
            return static_cast<MAP_TYPE>(map);
        }

        constexpr MAP_FLAGS GetMapFlags(MapFlags mapFlags) {
            return static_cast<MAP_FLAGS>(mapFlags);
        }

        static BUFFER_MODE GetBufferMode(GpuBufferMode mode) {
            return static_cast<BUFFER_MODE>(mode);
        }

        constexpr uint8_t GetSampleCount(SampleCount sampleCount) {
            return static_cast<uint8_t>(sampleCount);
        }

        static ImmutableSamplerDesc GetImmutableSampler(const ImmutableSamplerSpec& immutableSamplerSpec) {
            ImmutableSamplerDesc samplerDesc;
            samplerDesc.SamplerOrTextureName = immutableSamplerSpec.SamplerOrTextureName.data();
            samplerDesc.Desc = GetSamplerDesc(immutableSamplerSpec.Specification);
            samplerDesc.ShaderStages = GetShaderType(immutableSamplerSpec.ShaderStages);

            return samplerDesc;
        }

        constexpr PIPELINE_RESOURCE_FLAGS GetPipelineResourceFlags(PipelineResourceFlag resourceFlag) {
            return static_cast<PIPELINE_RESOURCE_FLAGS>(resourceFlag);
        }

        constexpr SHADER_RESOURCE_TYPE GetShaderResourceType(ShaderResourceType resourceType) {
            return static_cast<SHADER_RESOURCE_TYPE>(resourceType);
        }

        static PipelineResourceDesc GetPipelineResourceDesc(const PipelineResourceSpec& pipelineResourceSpec) {
            PipelineResourceDesc desc;
            desc.ShaderStages = GetShaderType(pipelineResourceSpec.ShaderStages);
            desc.Name = pipelineResourceSpec.Name.data();
            desc.ArraySize = pipelineResourceSpec.ArraySize;
            desc.Flags = GetPipelineResourceFlags(pipelineResourceSpec.Flags);
            desc.ResourceType = GetShaderResourceType(pipelineResourceSpec.ResourceType);
            desc.VarType = GetShaderResourceVariableType(pipelineResourceSpec.VariableType);

            return desc;
        }

        static PipelineResourceSignatureDesc GetPipelineResourceSignatureDesc(const PipelineResourceSignatureSpec& spec) {
            PipelineResourceSignatureDesc desc;

            desc.Name = spec.Name.data();

            Vec<ImmutableSamplerDesc> samplers;
            samplers.reserve(spec.ImmutableSamplers.size());
            for (const ImmutableSamplerSpec& immutableSampler : spec.ImmutableSamplers) {
                samplers.push_back(GetImmutableSampler(immutableSampler));
            }

            desc.ImmutableSamplers = samplers.data();
            desc.NumImmutableSamplers = samplers.size();

            Vec<PipelineResourceDesc> resources;
            resources.reserve(spec.Resources.size());
            for (const PipelineResourceSpec& resource : spec.Resources) {
                resources.push_back(GetPipelineResourceDesc(resource));
            }

            desc.Resources = resources.data();
            desc.NumResources = resources.size();

            desc.CombinedSamplerSuffix = spec.CombinedSamplerSuffix.data();
            desc.UseCombinedTextureSamplers = spec.UseCombinedTextureSamplers;
            desc.BindingIndex = spec.BindingIndex;
            desc.SRBAllocationGranularity = spec.SRBAllocationGranularity;

            return desc;
        }

        constexpr RESOURCE_STATE_TRANSITION_MODE GetResourceStateTransition(ResourceStateTransitionMode resourceStateTransitionMode) {
            return static_cast<RESOURCE_STATE_TRANSITION_MODE>(resourceStateTransitionMode);
        }

        constexpr TEXTURE_VIEW_TYPE GetTextureViewType(TextureViewType textureView) {
            return static_cast<TEXTURE_VIEW_TYPE>(textureView);
        }

        constexpr STENCIL_OP GetStencilOp(StencilOp stencilOp) {
            return static_cast<STENCIL_OP>(stencilOp);
        }

        StencilOpDesc GetStencilOp(const StencilOpSpec backFace) {
            StencilOpDesc desc;
            desc.StencilFailOp = GetStencilOp(backFace.StencilFailOp);
            desc.StencilDepthFailOp = GetStencilOp(backFace.StencilDepthFailOp);
            desc.StencilPassOp = GetStencilOp(backFace.StencilPassOp);
            desc.StencilFunc = GetComparisonFunc(backFace.StencilFunc);

            return desc;
        }

        constexpr BLEND_OPERATION GetBlendOperation(BlendOperation blendOp) {
            return static_cast<BLEND_OPERATION>(blendOp);
        }

        constexpr BLEND_FACTOR GetBlendFactor(BlendFactor blendFactor) {
            return static_cast<BLEND_FACTOR>(blendFactor);
        }

        constexpr LOGIC_OPERATION GetLogicOperation(LogicOperation logicOp) {
            return static_cast<LOGIC_OPERATION>(logicOp);
        }

        constexpr COLOR_MASK GetColorMask(ColorMask colorMask) {
            return static_cast<COLOR_MASK>(colorMask);
        }

        static BlendStateDesc GetBlendStateSpec(const BlendStateSpec& blendSpec) {
            BlendStateDesc desc;
            desc.AlphaToCoverageEnable = blendSpec.AlphaToCoverageEnable;
            desc.IndependentBlendEnable = blendSpec.IndependentBlendEnable;

            static_assert(k_MaxRenderTargets == DILIGENT_MAX_RENDER_TARGETS);

            for (uint32_t i = 0; i < k_MaxRenderTargets; i++) {
                const RenderTargetBlendSpec& renderTargetSpec = blendSpec.RenderTargets[i];
                RenderTargetBlendDesc& renderTargetDesc = desc.RenderTargets[i];

                renderTargetDesc.BlendEnable = renderTargetSpec.BlendEnable;
                renderTargetDesc.LogicOperationEnable = renderTargetSpec.LogicOperationEnable;
                renderTargetDesc.SrcBlend = GetBlendFactor(renderTargetSpec.SrcBlend);
                renderTargetDesc.DestBlend = GetBlendFactor(renderTargetSpec.DestBlend);
                renderTargetDesc.BlendOp = GetBlendOperation(renderTargetSpec.BlendOp);
                renderTargetDesc.SrcBlendAlpha = GetBlendFactor(renderTargetSpec.SrcBlendAlpha);
                renderTargetDesc.DestBlendAlpha = GetBlendFactor(renderTargetSpec.DestBlendAlpha);
                renderTargetDesc.BlendOpAlpha = GetBlendOperation(renderTargetSpec.BlendOpAlpha);
                renderTargetDesc.LogicOp = GetLogicOperation(renderTargetSpec.LogicOp);
                renderTargetDesc.RenderTargetWriteMask = GetColorMask(renderTargetSpec.RenderTargetWriteMask);
            }

            return desc;
        }

        constexpr UAV_ACCESS_FLAG GetUAVAccessFlags(UAVAccessFlags accessFlags) {
            return static_cast<UAV_ACCESS_FLAG>(accessFlags);
        }

        constexpr TEXTURE_VIEW_FLAGS GetTextureViewFlags(TextureViewFlags flags) {
            return static_cast<TEXTURE_VIEW_FLAGS>(flags);
        }

        constexpr BUFFER_VIEW_TYPE GetBufferViewType(BufferViewType bufferViewType) {
            return static_cast<BUFFER_VIEW_TYPE>(bufferViewType);
        }

        constexpr FILL_MODE GetFillMode(FillMode fillMode) {
            return static_cast<FILL_MODE>(fillMode);
        }

        constexpr SET_SHADER_RESOURCE_FLAGS GetSetShaderResourceFlag(SetShaderResourceFlag flags) {
            return static_cast<SET_SHADER_RESOURCE_FLAGS>(flags);
        }

        constexpr FENCE_TYPE GetFenceType(FenceType type) {
            return static_cast<FENCE_TYPE>(type);
        }
    }

    namespace TextureUtil {

        static RESOURCE_DIMENSION GetTextureType(const TextureType textureType) {
            switch (textureType) {
            case TextureType::Texture2D: return RESOURCE_DIM_TEX_2D;
            case TextureType::Texture2DArray: return RESOURCE_DIM_TEX_2D_ARRAY;
            case TextureType::TextureCube: return RESOURCE_DIM_TEX_CUBE;
            }

            return RESOURCE_DIM_UNDEFINED;
        }

        static void CreateTexture(IRenderDevice* device, QTextureData& textureData, const TextureData* data) {
            const TextureSpecification& spec = textureData.Specification;

            TextureDesc textureDesc;
            textureDesc.Name = spec.Name.c_str();
            textureDesc.Format = Utils::GetFormat(spec.Format);
            textureDesc.Width = spec.Width;
            textureDesc.Height = spec.Height;
            textureDesc.Type = GetTextureType(spec.Type);
            textureDesc.BindFlags = Utils::GetBindFlags(spec.BindFlags);
            textureDesc.SampleCount = Utils::GetSampleCount(spec.SampleCount);
            textureDesc.Usage = Utils::GetUsage(spec.Usage);
            textureDesc.CPUAccessFlags = Utils::GetCPUAccessFlags(spec.CpuAccessFlags);
            textureDesc.ArraySize = spec.ArraySize;
            textureDesc.MipLevels = spec.MipLevels;

            device->CreateTexture(textureDesc, data, &textureData.Texture);
        }

        static ITextureView* CreateTextureView(ITexture* texture, const TextureViewSpec& textureViewSpec) {
            TextureViewDesc desc;
            desc.ViewType = Utils::GetTextureViewType(textureViewSpec.ViewType);
            desc.TextureDim = GetTextureType(textureViewSpec.TextureType);
            desc.Format = Utils::GetFormat(textureViewSpec.Format);
            desc.MostDetailedMip = textureViewSpec.MostDetailedMip;
            desc.NumMipLevels = textureViewSpec.NumMipLevels;
            desc.FirstArraySlice = textureViewSpec.FirstArraySlice;
            desc.NumArraySlices = textureViewSpec.NumArraySlices;
            desc.AccessFlags = Utils::GetUAVAccessFlags(textureViewSpec.AccessFlags);
            desc.Flags = Utils::GetTextureViewFlags(textureViewSpec.Flags);

            ITextureView* textureView = nullptr;
            texture->CreateView(desc, &textureView);

            return textureView;
        }

        void Resize(
            IRenderDevice* device,
            QTextureData& data,
            const uint32_t width,
            const uint32_t height,
            ResourceTable<TextureViewData, TextureView>& textureViewTable
        ) {
            data.Texture->Release();
            data.Texture = nullptr;

            data.Specification.Width = width;
            data.Specification.Height = height;

            CreateTexture(device, data, nullptr);

            for (const auto& [textureView, isDefaultView] : data.TextureViews) {
                auto* textureViewData = textureViewTable.At(textureView);
                if (!textureViewData) {
                    continue;
                }

                if (isDefaultView) {
                    textureViewData->TextureView = data.Texture->GetDefaultView(
                        Utils::GetTextureViewType(textureViewData->Specification.ViewType)
                    );
                } else {
                    textureViewData->TextureView = CreateTextureView(data.Texture, textureViewData->Specification);
                }
            }
        }
    }

    DiligentRendererContext* DiligentRendererContext::s_Instance = nullptr;

    void DiligentRendererContext::Init(const Ref<Window>& window, RendererAPI api) {
        m_RendererAPI = api;
        m_Window = window;
        s_Instance = this;

        SetDebugMessageCallback([](DEBUG_MESSAGE_SEVERITY severity,
                                       const Char*            message,
                                       const char*            function,
                                       const char*            file,
                                       int                    line)
        {
            switch (severity)
            {
            case DEBUG_MESSAGE_SEVERITY_ERROR:
                QS_CORE_ERROR_TAG(
                    "DiligentRendererContext", "{}{}{}",
                    message ? message : "",
                    function ? FormatTemp(" {}", function) : "",
                    file ? FormatTemp(" (at {}:{})", file, line) : ""
                );
                break;
            case DEBUG_MESSAGE_SEVERITY_FATAL_ERROR:
                QS_CORE_CRITICAL_TAG("DiligentRendererContext", "{}{}{}",
                    message ? message : "",
                    function ? FormatTemp(" {}", function) : "",
                    file ? FormatTemp(" (at {}:{})", file, line) : ""
                );
                break;
            case DEBUG_MESSAGE_SEVERITY_WARNING:
                QS_CORE_WARN_TAG("DiligentRendererContext", "{}{}{}",
                    message ? message : "",
                    function ? FormatTemp(" {}", function) : "",
                    file ? FormatTemp(" (at {}:{})", file, line) : ""
                );
                break;
            case DEBUG_MESSAGE_SEVERITY_INFO:
                QS_CORE_TRACE_TAG("DiligentRendererContext", "{}{}{}",
                    message ? message : "",
                    function ? FormatTemp(" {}", function) : "",
                    file ? FormatTemp(" (at {}:{})", file, line) : ""
                );
                break;
            default:
                break;
            }
        });

        SwapChainDesc SCDesc;
        SCDesc.BufferCount = 3;
        SCDesc.Width = window->GetWidth();
        SCDesc.Height = window->GetHeight();
        SCDesc.ColorBufferFormat = TEX_FORMAT_RGBA8_UNORM;
        SCDesc.DepthBufferFormat = TEX_FORMAT_D32_FLOAT;

#if QS_PLATFORM_WINDOWS
        Win32NativeWindow nativeWindow;
        nativeWindow.hWnd = window->GetNativeWindow();
#elif QS_PLATFORM_LINUX
        LinuxNativeWindow nativeWindow;

        nativeWindow.pDisplay = window->GetNativeDisplay();
        if (window->IsWayland()) {
            nativeWindow.pWaylandSurface = window->GetNativeWindow();
        }
        else {
            nativeWindow.pXCBConnection = window->GetNativeWindow();
        }
#elif QS_PLATFORM_MACOS
        MacOSNativeWindow nativeWindow;
        nativeWindow.pNSView = Platform::GetNSViewFromWindow(window->GetNativeWindow());
#endif

        // TODO: Maybe fallback option?
        switch (api) {
#if D3D11_SUPPORTED
        case RendererAPI::Direct3D11: {
            auto* pFactoryD3D11 = LoadAndGetEngineFactoryD3D11();
            EngineD3D11CreateInfo engineCi;
            pFactoryD3D11->CreateDeviceAndContextsD3D11(engineCi, &m_pDevice, &m_pImmediateContext);
            pFactoryD3D11->CreateSwapChainD3D11(
                m_pDevice,
                m_pImmediateContext,
                SCDesc,
                FullScreenModeDesc{},
                nativeWindow,
                &m_pSwapChain
            );
            break;
        }
#endif
#if D3D12_SUPPORTED
        case RendererAPI::Direct3D12: {
            auto* pFactoryD3D12 = LoadAndGetEngineFactoryD3D12();
            EngineD3D12CreateInfo engineCi;

            engineCi.GPUDescriptorHeapDynamicSize[0] = 16384;

            pFactoryD3D12->CreateDeviceAndContextsD3D12(engineCi, &m_pDevice, &m_pImmediateContext);
            pFactoryD3D12->CreateSwapChainD3D12(
                m_pDevice,
                m_pImmediateContext,
                SCDesc,
                FullScreenModeDesc{},
                nativeWindow,
                &m_pSwapChain
            );
            break;
        }
#endif
#if VULKAN_SUPPORTED
        case RendererAPI::Vulkan: {
            auto* pFactoryVk = GetEngineFactoryVk();
            EngineVkCreateInfo engineCi;
            engineCi.GraphicsAPIVersion = Version{1, 3};

            engineCi.DynamicHeapSize = 32 << 20;
            engineCi.EnableValidation = true;

#ifndef QS_PLATFORM_MACOS
            VkPhysicalDeviceVulkan12Features vk12Features{};
            vk12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
            vk12Features.drawIndirectCount = VK_TRUE;
            vk12Features.runtimeDescriptorArray   = VK_TRUE;
            vk12Features.descriptorBindingPartiallyBound = VK_TRUE;
            vk12Features.pNext = nullptr;
#endif

            VkPhysicalDeviceVulkan11Features vk11Features{};
            vk11Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
            vk11Features.shaderDrawParameters = VK_TRUE;
#ifndef QS_PLATFORM_MACOS
            vk11Features.pNext = &vk12Features;
#endif

            engineCi.pDeviceExtensionFeatures = &vk11Features;

            pFactoryVk->CreateDeviceAndContextsVk(
                engineCi,
                &m_pDevice,
                &m_pImmediateContext
            );
            pFactoryVk->CreateSwapChainVk(
                m_pDevice,
                m_pImmediateContext,
                SCDesc,
                nativeWindow,
                &m_pSwapChain
            );
            break;
        }
#endif
#if GL_SUPPORTED
        case RendererAPI::OpenGL: {
            // Declare function pointer
            auto* pFactoryOpenGL = GetEngineFactoryOpenGL();

            EngineGLCreateInfo engineCi;
            engineCi.Window = nativeWindow;

            pFactoryOpenGL->CreateDeviceAndSwapChainGL(
                engineCi,
                &m_pDevice,
                &m_pImmediateContext,
                SCDesc,
                &m_pSwapChain
            );

            break;
        }
#endif
        default:
            QS_CORE_ASSERT(false, "No supported renderer found");
            break;
        }

        // CREATE RESOURCES
    }

    RendererAPI DiligentRendererContext::GetRendererAPI() {
        return m_RendererAPI;
    }

    bool DiligentRendererContext::HomogenousDepth() {
        switch (m_RendererAPI) {
        case RendererAPI::None:
        case RendererAPI::OpenGL:
            return false;
        case RendererAPI::Vulkan:
        case RendererAPI::Direct3D11:
        case RendererAPI::Direct3D12:
        case RendererAPI::Metal:
            return true;
        }

        return false;
    }

    void DiligentRendererContext::StartFrame() {
        // Set render targets before issuing any draw command.
        // Note that Present() unbinds the back buffer if it is set as render target.
        auto* pRTV = m_pSwapChain->GetCurrentBackBufferRTV();
        auto* pDSV = m_pSwapChain->GetDepthBufferDSV();
        m_pImmediateContext->SetRenderTargets(1, &pRTV, pDSV, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        // Clear the back buffer
        const float4 clearColor(0.350f, 0.350f, 0.350f, 1.0f);
        // Let the engine perform required state transitions
        m_pImmediateContext->ClearRenderTarget(
            pRTV,
            math::value_ptr(clearColor),
            RESOURCE_STATE_TRANSITION_MODE_TRANSITION
        );

        m_pImmediateContext->ClearDepthStencil(
            pDSV,
            CLEAR_DEPTH_FLAG,
            1.f,
            0,
            RESOURCE_STATE_TRANSITION_MODE_TRANSITION
        );
    }

    void DiligentRendererContext::EndFrame() {
        m_pImmediateContext->Flush();
        m_pSwapChain->Present(1);
    }

    void DiligentRendererContext::Reset(const uint32_t width, const uint32_t height) {
        if (m_pSwapChain) {
            m_pSwapChain->Resize(width, height);
        }
    }

    void DiligentRendererContext::StartSceneRender(const float4x4& view, const float4x4& projection) {
        /*{
            // Map the buffer and write current world-view-projection matrix
            MapHelper<float4x4> CBConstants(m_pImmediateContext, m_VSConstants, MAP_WRITE, MAP_FLAG_DISCARD);
            *CBConstants = math::mul(view, projection);
        }*/
    }

    void DiligentRendererContext::BeginRenderPass(const BeginRenderPassAttribs& beginRenderPassAttrib) {
        SmallVec<OptimizedClearValue, 2> clearValues;
        clearValues.reserve(beginRenderPassAttrib.ClearColors.size());
        for (const ClearValue& clearColor : beginRenderPassAttrib.ClearColors) {
            OptimizedClearValue optimizedClearValue;
            optimizedClearValue.Format = Utils::GetFormat(clearColor.Format);
            math::store(optimizedClearValue.Color, clearColor.Color);
            optimizedClearValue.DepthStencil.Depth = clearColor.DepthStencil.Depth;
            optimizedClearValue.DepthStencil.Stencil = clearColor.DepthStencil.Stencil;

            clearValues.push_back(optimizedClearValue);
        }

        Diligent::BeginRenderPassAttribs attribs;
        attribs.pClearValues = clearValues.data();
        attribs.ClearValueCount = clearValues.size();
        attribs.pFramebuffer = m_FrameBufferTable.At(beginRenderPassAttrib.FrameBufferHandle)->FrameBuffer;
        attribs.pRenderPass = m_RenderPassTable.At(beginRenderPassAttrib.RenderPassHandle)->RenderPass;
        attribs.StateTransitionMode = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;

        m_pImmediateContext->BeginRenderPass(attribs);
    }

    void DiligentRendererContext::EndRenderPass() {
        m_pImmediateContext->EndRenderPass();
    }

    GpuBufferHandle DiligentRendererContext::CreateBuffer(const GpuBufferSpec& bufferSpec, const BufferView data) {
        const Handle<GpuBuffer> handle = m_BufferTable.Emplace();
        QBufferData* slot = m_BufferTable.At(handle);
        slot->Name = bufferSpec.Name;
        slot->Specification = bufferSpec;
        slot->Specification.Name = slot->Name;

        BufferDesc desc;
        desc.Name = slot->Name.c_str();
        desc.BindFlags = Utils::GetBindFlags(bufferSpec.BindFlags);
        desc.CPUAccessFlags = Utils::GetCPUAccessFlags(bufferSpec.CpuAccessFlags);
        desc.MiscFlags = Utils::GetMiscFlags(bufferSpec.MiscFlags);
        desc.Usage = Utils::GetUsage(bufferSpec.Usage);
        desc.Mode = Utils::GetBufferMode(bufferSpec.Mode);
        desc.Size = bufferSpec.Size;
        desc.ElementByteStride = bufferSpec.ElementByteStride;
        desc.ImmediateContextMask = bufferSpec.ImmediateContextMask;

        BufferData bufferData;
        bufferData.pData = data.data();
        bufferData.DataSize = bufferSpec.Size;

        m_pDevice->CreateBuffer(desc, bufferData.pData ? &bufferData : nullptr, &slot->Buffer);

        return handle;
    }

    GpuBufferViewHandle DiligentRendererContext::GetDefaultBufferView(
        const GpuBufferHandle bufferHandle,
        const BufferViewType bufferViewType
    ) {
        QBufferData* slot = m_BufferTable.At(bufferHandle);

        if (!slot) [[unlikely]] {
            QS_CORE_ERROR_TAG(
                "DiligentRendererContext",
                "Failed to find buffer in slot ({}, {})",
                bufferHandle.Index(), bufferHandle.Generation()
            );

            return {};
        }

        IBufferView* bufferView = slot->Buffer->GetDefaultView(Utils::GetBufferViewType(bufferViewType));

        if (!bufferView) {
            QS_CORE_ERROR_TAG(
                "DiligentRendererContext",
                "Failed to get Default buffer view BUFFER_VIEW_SHADER_RESOURCE from buffer {}!",
                slot->Name
            );

            return {};
        }

        const GpuBufferViewHandle handle = m_BufferViewTable.Emplace();

        QBufferViewData* viewSlot = m_BufferViewTable.At(handle);
        viewSlot->BufferView = bufferView;
        viewSlot->Buffer = bufferHandle;

        slot->BufferViews.push_back(handle);

        return handle;
    }

    void DiligentRendererContext::CopyBuffer(
        const GpuBufferHandle srcBuffer,
        const uint64_t srcOffset,
        const ResourceStateTransitionMode srcBufferTransitionMode,
        const GpuBufferHandle dstBuffer,
        const uint64_t dstOffset,
        const uint64_t size,
        const ResourceStateTransitionMode dstBufferTransitionMode
    ) {
        const QBufferData* srcSlot = m_BufferTable.At(srcBuffer);
        const QBufferData* dstSlot = m_BufferTable.At(dstBuffer);

        if (!srcSlot && !dstSlot) [[unlikely]] {
            QS_CORE_ERROR_TAG(
                "DiligentRendererContext",
                "Invalids buffers passed in! Src: ({},{}), Dst: ({},{})",
                srcBuffer.Index(), srcBuffer.Generation(), dstBuffer.Index(), dstBuffer.Generation()
            );

            return;
        }

        m_pImmediateContext->CopyBuffer(
            srcSlot->Buffer,
            srcOffset,
            Utils::GetResourceStateTransition(srcBufferTransitionMode),
            dstSlot->Buffer,
            dstOffset,
            size,
            Utils::GetResourceStateTransition(dstBufferTransitionMode)
        );
    }

    ShaderResourceBindingHandle DiligentRendererContext::CreateShaderResourceBinding(
        PipelineStateHandle pipelineStateHandle, bool initStaticResources
    ) {
        Handle<ShaderResourceBinding> handle = m_ShaderResourceBindingTable.Emplace();
        IShaderResourceBinding** slot = m_ShaderResourceBindingTable.At(handle);

        QS_CORE_ASSERT(slot);

        IPipelineState* pipelineState = m_PipelineStateTable.At(pipelineStateHandle)->PSO;
        pipelineState->CreateShaderResourceBinding(slot, initStaticResources);

        return handle;
    }

    void DiligentRendererContext::BindVariableByName(
        ShaderType shaderType,
        ShaderResourceBindingHandle shaderResourceBindingHandle,
        std::string_view name,
        GpuBufferViewHandle gpuBufferViewHandle,
        SetShaderResourceFlag flags
    ) {
        IShaderResourceBinding** slot = m_ShaderResourceBindingTable.At(shaderResourceBindingHandle);

        if (!slot) [[unlikely]] {
            QS_CORE_ERROR_TAG(
                "DiligentRendererContext",
                "Failed to find shader resource binding in slot ({},{})",
                shaderResourceBindingHandle.Index(),
                shaderResourceBindingHandle.Generation()
            );

            return;
        }

        IShaderResourceVariable* shaderResourceVariable = (*slot)->GetVariableByName(
            Utils::GetShaderType(shaderType),
            FormatTemp("{}", name)
        );

        if (!shaderResourceVariable) [[unlikely]] {
            QS_CORE_ERROR_TAG(
                "DiligentRenderer",
                "Failed to find shader variable by the name '{}'!",
                name
            );

            return;
        }

        shaderResourceVariable->Set(
            m_BufferViewTable.At(gpuBufferViewHandle)->BufferView,
            Utils::GetSetShaderResourceFlag(flags)
        );
    }

    void DiligentRendererContext::BindVariableByName(
        const ShaderType shaderType,
        const ShaderResourceBindingHandle shaderResourceBindingHandle,
        std::string_view name,
        const TextureViewHandle textureViewHandle,
        const SetShaderResourceFlag flags
    ) {
        IShaderResourceBinding** slot = m_ShaderResourceBindingTable.At(shaderResourceBindingHandle);

        if (!slot) [[unlikely]] {
            QS_CORE_ERROR_TAG(
                "DiligentRendererContext",
                "Failed to find shader resource binding in slot ({},{})",
                shaderResourceBindingHandle.Index(),
                shaderResourceBindingHandle.Generation()
            );

            return;
        }

        IShaderResourceVariable* shaderResourceVariable = (*slot)->GetVariableByName(
            Utils::GetShaderType(shaderType),
            FormatTemp("{}", name)
        );

        if (!shaderResourceVariable) [[unlikely]] {
            QS_CORE_ERROR_TAG(
                "DiligentRenderer",
                "Failed to find shader variable by the name '{}'!",
                name
            );

            return;
        }

        shaderResourceVariable->Set(
            m_TextureViewTable.At(textureViewHandle)->TextureView,
            Utils::GetSetShaderResourceFlag(flags)
        );
    }

    void DiligentRendererContext::BindArrayByName(
        const ShaderType shaderType,
        const ShaderResourceBindingHandle shaderResourceBindingHandle,
        std::string_view name,
        const Span32<const uint64_t> nativeTextureHandles,
        const SetShaderResourceFlag flags
    ) {
        IShaderResourceBinding** slot = m_ShaderResourceBindingTable.At(shaderResourceBindingHandle);

        if (!slot) [[unlikely]] {
            QS_CORE_ERROR_TAG(
                "DiligentRendererContext",
                "Failed to find shader resource binding in slot ({},{})",
                shaderResourceBindingHandle.Index(),
                shaderResourceBindingHandle.Generation()
            );

            return;
        }

        IShaderResourceVariable* shaderResourceVariable = (*slot)->GetVariableByName(
            Utils::GetShaderType(shaderType),
            FormatTemp("{}", name)
        );

        if (!shaderResourceVariable) {
            /*
            TODO: No way to really know
            QS_CORE_ERROR_TAG(
                "DiligentRenderer",
                "Failed to find shader variable by the name '{}'!",
                name
            );
            */

            return;
        }

        shaderResourceVariable->SetArray(
            reinterpret_cast<IDeviceObject* const*>(nativeTextureHandles.data()),
            0,
            nativeTextureHandles.size(),
            Utils::GetSetShaderResourceFlag(flags)
        );
    }

    void DiligentRendererContext::Map(
        GpuBufferHandle bufferHandle, MapType mapType, MapFlags mapFlags, void*& mappedData
    ) {
        m_pImmediateContext->MapBuffer(
            m_BufferTable.At(bufferHandle)->Buffer,
            Utils::GetMapType(mapType),
            Utils::GetMapFlags(mapFlags),
            mappedData
        );
    }

    void DiligentRendererContext::Unmap(const GpuBufferHandle bufferHandle, const MapType mapType) {
        m_pImmediateContext->UnmapBuffer(m_BufferTable.At(bufferHandle)->Buffer, Utils::GetMapType(mapType));
    }

    void DiligentRendererContext::CommitShaderResources(
        const ShaderResourceBindingHandle shaderResourceBindingHandle,
        const ResourceStateTransitionMode resourceStateTransitionMode
    ) {
        IShaderResourceBinding** slot = m_ShaderResourceBindingTable.At(shaderResourceBindingHandle);
        if (!slot) [[unlikely]] {
            QS_CORE_ERROR_TAG(
                "DiligentRendererContext",
                "CommitShaderResources: Failed to get slot for ShaderResourceBinding at handle ({},{})",
                shaderResourceBindingHandle.Index(), shaderResourceBindingHandle.Generation()
            );

            return;
        }

        m_pImmediateContext->CommitShaderResources(
            *slot,
            Utils::GetResourceStateTransition(resourceStateTransitionMode)
        );
    }

    void DiligentRendererContext::Destroy(const ShaderResourceBindingHandle shaderResourceBindingHandle) {
        IShaderResourceBinding** slot = m_ShaderResourceBindingTable.At(shaderResourceBindingHandle);

        if (!slot) [[unlikely]] {
            QS_CORE_ERROR_TAG(
                "DiligentRendererContext",
                "Failed to find shader resource binding in slot ({},{})",
                shaderResourceBindingHandle.Index(),
                shaderResourceBindingHandle.Generation()
            );

            return;
        }

        (*slot)->Release();
        *slot = nullptr;

        m_ShaderResourceBindingTable.Erase(shaderResourceBindingHandle);
    }

    void DiligentRendererContext::UpdateBuffer(GpuBufferHandle gpuBufferHandle, uint64_t offset, BufferView data) {
        m_pImmediateContext->UpdateBuffer(
            m_BufferTable.At(gpuBufferHandle)->Buffer,
            // buffer
            offset,
            // offset
            data.size(),
            // size
            data.data(),
            // data pointer
            RESOURCE_STATE_TRANSITION_MODE_TRANSITION
        );
    }

    void DiligentRendererContext::Destroy(GpuBufferHandle bufferHandle) {
        QBufferData* slot = m_BufferTable.At(bufferHandle);

        for (const GpuBufferViewHandle viewHandle : slot->BufferViews) {
            m_BufferViewTable.Erase(viewHandle);
        }

        slot->Buffer->Release();
        slot->Buffer = nullptr;
        slot->Specification = {};
        slot->Name.clear();
        slot->BufferViews.clear();

        m_BufferTable.Erase(bufferHandle);
    }

    void DiligentRendererContext::TransitionShaderResources(ShaderResourceBindingHandle shaderResourceBindingHandle) {
        IShaderResourceBinding** slot = m_ShaderResourceBindingTable.At(shaderResourceBindingHandle);
        QS_CORE_ASSERT(slot);

        m_pImmediateContext->TransitionShaderResources(*slot);
    }

    void DiligentRendererContext::TransitionResource(const TextureHandle textureHandle, ResourceState resourceState) {
        StateTransitionDesc barrier;
        barrier.pResource = m_TextureTable.At(textureHandle)->Texture;
        barrier.OldState = RESOURCE_STATE_UNKNOWN;
        barrier.NewState = Utils::GetResourceState(resourceState);
        barrier.FirstArraySlice = 0;
        barrier.ArraySliceCount = REMAINING_ARRAY_SLICES;
        barrier.Flags = STATE_TRANSITION_FLAG_UPDATE_STATE;

        m_pImmediateContext->TransitionResourceState(barrier);
    }

    void DiligentRendererContext::Destroy(VertexBufferHandle vertexBufferHandle) {
        IBuffer** slot = m_VertexBufferTable.At(vertexBufferHandle);
        if (!slot) [[unlikely]] {
            QS_CORE_ERROR_TAG(
                "DiligentRendererContext",
                "Failed to get vertex buffer slot at handle ({},{})",
                vertexBufferHandle.Index(), vertexBufferHandle.Generation()
            );

            return;
        }

        (*slot)->Release();
        *slot = nullptr;

        m_VertexBufferTable.Erase(vertexBufferHandle);
    }

    UniformBufferHandle DiligentRendererContext::CreateUniformBuffer(
        const std::string& name,
        UniformBufferType uniformType, uint32_t count
    ) {
        // TODO:
        return {};
    }

    void DiligentRendererContext::SetUniformData(
        UniformBufferHandle uniformBufferHandle, const void* data,
        uint32_t count
    ) {
        // TODO:
    }

    void DiligentRendererContext::Destroy(UniformBufferHandle uniformBufferHandle) {
        // TODO:
    }

    TextureHandle DiligentRendererContext::CreateTexture(const TextureSpecification& spec) {
        const Handle<Texture> handle = m_TextureTable.Emplace();
        QTextureData* slot = m_TextureTable.At(handle);
        slot->Specification = spec;

        TextureUtil::CreateTexture(m_pDevice, *slot, nullptr);

        return handle;
    }

    TextureHandle DiligentRendererContext::CreateTexture(const TextureSpecification& spec, const BufferView data) {
        const Handle<Texture> handle = m_TextureTable.Emplace();
        QTextureData* slot = m_TextureTable.At(handle);
        slot->Specification = spec;

        TextureSubResData subRes{};
        subRes.pData = data.data();
        subRes.Stride = spec.Width * 4; // 4 bytes per pixel (RGBA)

        TextureData textureData;
        textureData.pSubResources = &subRes;
        textureData.NumSubresources = 1;

        TextureUtil::CreateTexture(m_pDevice, *slot, &textureData);

        return handle;
    }

    TextureHandle DiligentRendererContext::CreateTexture(
        const TextureSpecification& spec,
        const std::filesystem::path& path
    ) {
        // TODO:
        return {};
    }

    bool DiligentRendererContext::TextureIsVFlipped() {
        // TODO:
        return false;
    }

    void DiligentRendererContext::TextureResize(
        const TextureHandle textureHandle,
        const uint32_t width,
        const uint32_t height
    ) {
        TextureUtil::Resize(m_pDevice, *m_TextureTable.At(textureHandle), width, height, m_TextureViewTable);
    }

    const TextureSpecification* DiligentRendererContext::GetSpecification(const TextureHandle textureHandle) {
        const auto* data = m_TextureTable.At(textureHandle);
        if (!data) {
            return nullptr;
        }

        return &data->Specification;
    }

    uint64_t DiligentRendererContext::TextureGetNativeHandle(const TextureHandle textureHandle) {
        const auto* data = m_TextureTable.At(textureHandle);
        return reinterpret_cast<uint64_t>(data->Texture->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));
    }

    TextureHandle DiligentRendererContext::GetTexture(const TextureViewHandle textureView) {
        const auto* textureViewData = m_TextureViewTable.At(textureView);
        if (!textureViewData) [[unlikely]] {
            return {};
        }

        return textureViewData->Texture;
    }

    TextureViewHandle DiligentRendererContext::TextureGetView(
        const TextureHandle texture, const TextureViewType textureViewType
    ) {
        auto* textureData = m_TextureTable.At(texture);
        if (!textureData) [[unlikely]] {
            return {};
        }

        const auto it = std::ranges::find_if(
            textureData->TextureViews,
            [&](const Pair<TextureViewHandle, bool>& view) {
                if (!view.second) {
                    return false;
                }

                return m_TextureViewTable.At(view.first)->Specification.ViewType == textureViewType;
            }
        );

        if (it != textureData->TextureViews.end()) {
            return it->first;
        }

        const TextureViewHandle handle = m_TextureViewTable.Emplace();
        TextureViewData* slot = m_TextureViewTable.At(handle);
        slot->Texture = texture;
        slot->Specification.ViewType = textureViewType;
        slot->Specification.TextureType = textureData->Specification.Type;
        slot->Specification.Format = textureData->Specification.Format;

        slot->TextureView = textureData->Texture->GetDefaultView(Utils::GetTextureViewType(textureViewType));
        textureData->TextureViews.emplace_back(handle, true);

        return handle;
    }

    TextureViewHandle DiligentRendererContext::TextureCreateView(
        TextureHandle texture, TextureViewSpec textureViewSpec
    ) {
        QTextureData* slot = m_TextureTable.At(texture);

        if (!slot) [[unlikely]] {
            QS_CORE_ERROR_TAG(
                "DiligentRendererContext",
                "Failed to find texture with handle ({},{})",
                texture.Index(), texture.Generation()
            );

            return {};
        }

        const TextureViewHandle handle = m_TextureViewTable.Emplace();
        TextureViewData* viewSlot = m_TextureViewTable.At(handle);

        QS_CORE_ASSERT(viewSlot);

        viewSlot->Texture = texture;
        viewSlot->Specification = textureViewSpec;

        viewSlot->TextureView = TextureUtil::CreateTextureView(slot->Texture, textureViewSpec);

        slot->TextureViews.emplace_back(handle, false);

        return handle;
    }

    void DiligentRendererContext::CopyTexture(const CopyTextureAttribs& copyAttribs) {
        Diligent::CopyTextureAttribs copy{};
        copy.pSrcTexture              = m_TextureTable.At(copyAttribs.Source)->Texture;
        copy.pDstTexture              = m_TextureTable.At(copyAttribs.Destination)->Texture;
        copy.SrcTextureTransitionMode = Utils::GetResourceStateTransition(copyAttribs.SourceTransitionMode);
        copy.DstTextureTransitionMode = Utils::GetResourceStateTransition(copyAttribs.DestinationTransitionMode);
        m_pImmediateContext->CopyTexture(copy);
    }

    void DiligentRendererContext::MapTextureSubresource(
        TextureHandle textureHandle, uint32_t mipLevel, uint32_t arraySlice, MapType mapType, MapFlags mapFlags,
        MappedTextureSubresource& mappedData
    ) {
        Diligent::MappedTextureSubresource mapped;
        m_pImmediateContext->MapTextureSubresource(
            m_TextureTable.At(textureHandle)->Texture,
            mipLevel,
            arraySlice,
            Utils::GetMapType(mapType),
            Utils::GetMapFlags(mapFlags),
            nullptr,
            mapped
        );

        mappedData.Data = mapped.pData;
        mappedData.Stride = mapped.Stride;
        mappedData.DepthStride = mapped.DepthStride;
    }

    void DiligentRendererContext::UnmapTextureSubresource(
        const TextureHandle textureHandle,
        const uint32_t mipLevel,
        const uint32_t arraySlice
    ) {
        m_pImmediateContext->UnmapTextureSubresource(m_TextureTable.At(textureHandle)->Texture, mipLevel, arraySlice);
    }

    void DiligentRendererContext::Bind(TextureHandle textureHandle) {
        // TODO:
    }

    void DiligentRendererContext::Destroy(const TextureHandle textureHandle) {
        QTextureData* slot = m_TextureTable.At(textureHandle);
        if (!slot) [[unlikely]] {
            QS_CORE_ERROR_TAG(
                "DiligentRendererContext",
                "Destroy(TextureHandle) Failed to get texture at handle ({},{})",
                textureHandle.Index(), textureHandle.Generation()
            );

            return;
        }

        for (const TextureViewHandle& textureView : slot->TextureViews | std::views::keys) {
            m_TextureViewTable.Erase(textureView);
        }

        slot->Texture->Release();
        m_TextureTable.Erase(textureHandle);
    }

    RenderPassHandle DiligentRendererContext::CreateRenderPass(const RenderPassSpec& renderPassSpec) {
        const Handle<RenderPass> handle = m_RenderPassTable.Emplace();
        RenderPassData* slot = m_RenderPassTable.At(handle);

        QS_CORE_ASSERT(slot);

        // Own data
        slot->Name = renderPassSpec.Name;
        slot->Attachments = SmallVec<RenderPassAttachmentSpec, 2>(renderPassSpec.Attachments);

        uint32_t renderTargetAttachmentRefsCount = 0;
        for (const SubPassSpec& subPass : renderPassSpec.SubPasses) {
            renderTargetAttachmentRefsCount += subPass.RenderTargetAttachments.size();
            if (subPass.pResolveAttachments) {
                renderTargetAttachmentRefsCount += subPass.RenderTargetAttachments.size();
            }
        }

        slot->AttachmentReferences.reserve(renderTargetAttachmentRefsCount);

        for (const SubPassSpec& subPass : renderPassSpec.SubPasses) {
            SubPassSpec ownedSubpass;

            const uint32_t attachmentIndex = slot->AttachmentReferences.size();
            std::ranges::copy(
                subPass.RenderTargetAttachments,
                std::back_inserter(slot->AttachmentReferences)
            );

            ownedSubpass.RenderTargetAttachments = Span32(
                slot->AttachmentReferences.data() + attachmentIndex,
                subPass.RenderTargetAttachments.size()
            );

            if (subPass.pResolveAttachments) {
                const uint32_t resolveAttachmentIndex = slot->AttachmentReferences.size();
                for (uint32_t i = 0; i < subPass.RenderTargetAttachments.size(); i++) {
                    slot->AttachmentReferences.push_back(subPass.pResolveAttachments[i]);
                }

                ownedSubpass.pResolveAttachments = &slot->AttachmentReferences[resolveAttachmentIndex];
            }

            ownedSubpass.pDepthAttachment = subPass.pDepthAttachment;

            slot->SubPasses.push_back(ownedSubpass);
        }

        // Set up the spec
        auto& spec = slot->Specification;
        spec.Name = slot->Name;
        spec.SubPasses = slot->SubPasses;
        spec.Attachments = slot->Attachments;

        SmallVec<RenderPassAttachmentDesc, 3> attachments;

        for (const RenderPassAttachmentSpec& attachment : spec.Attachments) {
            attachments.push_back(Utils::GetRenderPassAttachmentDesc(attachment));
        }

        SmallVec<SubpassDesc, 2> subpasses;
        SmallVec<Diligent::AttachmentReference, 3> attachmentRefs;

        uint32_t totalAttachmentRefCount = 0;
        for (const SubPassSpec& pass : spec.SubPasses) {
            totalAttachmentRefCount += pass.RenderTargetAttachments.size() + 1;
            if (pass.pResolveAttachments) {
                totalAttachmentRefCount += pass.RenderTargetAttachments.size();
            }
        }

        attachmentRefs.reserve(totalAttachmentRefCount);
        subpasses.reserve(spec.SubPasses.size());

        for (const SubPassSpec& pass : spec.SubPasses) {
            SubpassDesc subPassDesc{};

            const uint32_t renderAttachmentIndex = attachmentRefs.size();
            for (const AttachmentReference& attachmentReference : pass.RenderTargetAttachments) {
                attachmentRefs.push_back(Utils::GetAttachmentReference(attachmentReference));
            }

            subPassDesc.pRenderTargetAttachments = attachmentRefs.data() + renderAttachmentIndex;
            subPassDesc.RenderTargetAttachmentCount = pass.RenderTargetAttachments.size();

            if (pass.pResolveAttachments) {
                const uint32_t resolveAttachmentIndex = attachmentRefs.size();
                for (uint32_t i = 0; i < pass.RenderTargetAttachments.size(); i++) {
                    attachmentRefs.push_back(Utils::GetAttachmentReference(pass.pResolveAttachments[i]));
                }

                subPassDesc.pResolveAttachments = attachmentRefs.data() + resolveAttachmentIndex;
            }

            if (pass.pDepthAttachment) {
                attachmentRefs.push_back(Utils::GetAttachmentReference(*pass.pDepthAttachment));
                subPassDesc.pDepthStencilAttachment = &attachmentRefs.back();
            }

            subpasses.push_back(subPassDesc);
        }

        RenderPassDesc RPDesc;
        RPDesc.Name = slot->Name.c_str();
        RPDesc.AttachmentCount = attachments.size();
        RPDesc.pAttachments = attachments.data();
        RPDesc.SubpassCount = subpasses.size();
        RPDesc.pSubpasses = subpasses.data();

        m_pDevice->CreateRenderPass(RPDesc, &slot->RenderPass);

        return handle;
    }

    void DiligentRendererContext::Destroy(const RenderPassHandle renderPassHandle) {
        RenderPassData* slot = m_RenderPassTable.At(renderPassHandle);
        slot->RenderPass->Release();
        slot->RenderPass = nullptr;
        slot->Specification = {};
        slot->Attachments.clear();
        slot->SubPasses.clear();
        slot->Name.clear();

        m_RenderPassTable.Erase(renderPassHandle);
    }


    FrameBufferHandle DiligentRendererContext::CreateFrameBuffer(const FrameBufferSpec& frameBufferSpec) {
        const Handle<FrameBuffer> handle = m_FrameBufferTable.Emplace();
        QFrameBufferData* slot = m_FrameBufferTable.At(handle);

        // Own data
        slot->Name = frameBufferSpec.Name;
        slot->Attachments = SmallVec<TextureViewHandle, 2>(frameBufferSpec.Attachments);

        FrameBufferSpec& spec = slot->Specification;
        spec = {slot->Name, slot->Attachments, frameBufferSpec.RenderPassHandle, frameBufferSpec.NumArraySlices, frameBufferSpec.Size};

        SmallVec<ITextureView*, 2> textureAttachments;
        textureAttachments.reserve(spec.Attachments.size());
        for (const TextureViewHandle attachment : spec.Attachments) {
            textureAttachments.push_back(m_TextureViewTable.At(attachment)->TextureView);
        }

        auto& textureDesc = textureAttachments[0]->GetTexture()->GetDesc();
        QS_CORE_ASSERT(
            spec.Size.Width == textureDesc.Width && spec.Size.Height == textureDesc.Height,
            "Frame buffer size does not match texture size!"
        );

        Utils::CreateFrameBuffer(
            slot->FrameBuffer,
            m_pDevice,
            slot->Name.c_str(),
            Span32(textureAttachments),
            spec.NumArraySlices,
            m_RenderPassTable.At(spec.RenderPassHandle)->RenderPass
        );

        return handle;
    }

    uint32_t DiligentRendererContext::FrameBufferGetWidth(const FrameBufferHandle frameBufferHandle) {
        const QFrameBufferData* slot = m_FrameBufferTable.At(frameBufferHandle);
        return slot->Specification.Size.Width;
    }

    uint32_t DiligentRendererContext::FrameBufferGetHeight(const FrameBufferHandle frameBufferHandle) {
        const QFrameBufferData* slot = m_FrameBufferTable.At(frameBufferHandle);
        return slot->Specification.Size.Height;
    }

    Extent2D DiligentRendererContext::FrameBufferGetSize(const FrameBufferHandle frameBufferHandle) {
        const QFrameBufferData* slot = m_FrameBufferTable.At(frameBufferHandle);
        return slot->Specification.Size;
    }

    void DiligentRendererContext::FrameBufferResize(
        const FrameBufferHandle frameBufferHandle,
        const uint32_t width,
        const uint32_t height
    ) {
        QFrameBufferData* data = m_FrameBufferTable.At(frameBufferHandle);
        data->Specification.Size = {width, height};

        SmallVec<ITextureView*, 2> textureViews;
        for (const TextureViewHandle attachment : data->Attachments) {
            const TextureViewData* textureViewData = m_TextureViewTable.At(attachment);
            textureViews.push_back(textureViewData->TextureView);
        }

        data->FrameBuffer->Release();
        data->FrameBuffer = nullptr;

        IRenderPass* renderPass = nullptr;
        if (data->Specification.RenderPassHandle.IsValid()) {
            renderPass = m_RenderPassTable.At(data->Specification.RenderPassHandle)->RenderPass;
        }

        Utils::CreateFrameBuffer(
            data->FrameBuffer,
            m_pDevice,
            data->Name.c_str(),
            Span32(textureViews),
            data->Specification.NumArraySlices,
            renderPass
        );
    }

    void DiligentRendererContext::Destroy(const FrameBufferHandle frameBufferHandle) {
        QFrameBufferData* data = m_FrameBufferTable.At(frameBufferHandle);
        data->FrameBuffer->Release();
        data->FrameBuffer = nullptr;
        data->Specification = {};
        data->Attachments.clear();
        data->Name.clear();

        m_FrameBufferTable.Erase(frameBufferHandle);
    }

    void DiligentRendererContext::SubmitMesh(
        const MeshRenderer& mesh,
        const WorldTransform& transform
    ) {
        Mesh& meshAsset = mesh.Mesh.Get();
        BindVertexBuffer(meshAsset.GetVertexBuffer(), 0);
        BindIndexBuffer(meshAsset.GetIndexBuffer());

        Diligent::DrawIndexedAttribs drawAttrs; // This is an indexed draw call
        drawAttrs.IndexType = VT_UINT16; // Index type
        drawAttrs.NumIndices = meshAsset.GetIndices().size();

        // Verify the state of vertex and index buffers
        drawAttrs.Flags = DRAW_FLAG_VERIFY_ALL;
        m_pImmediateContext->DrawIndexed(drawAttrs);
    }

    void DiligentRendererContext::Draw(const DrawAttribs& drawAttribs) {
        Diligent::DrawAttribs attribs;
        attribs.NumVertices = drawAttribs.NumVertices;
        attribs.Flags = Utils::GetDrawFlags(drawAttribs.Flags);
        attribs.NumInstances = drawAttribs.NumInstances;
        attribs.StartVertexLocation = drawAttribs.StartVertexLocation;
        attribs.FirstInstanceLocation = drawAttribs.FirstInstanceLocation;

        m_pImmediateContext->Draw(attribs);
    }
    void DiligentRendererContext::DrawIndexed(const DrawIndexedAttribs& drawIndexedAttribs) {
        Diligent::DrawIndexedAttribs attribs;
        attribs.Flags = Utils::GetDrawFlags(drawIndexedAttribs.Flags);
        attribs.IndexType = Utils::GetValueType(drawIndexedAttribs.IndexType);
        attribs.NumIndices = drawIndexedAttribs.NumIndices;
        attribs.BaseVertex = drawIndexedAttribs.BaseVertex;
        attribs.FirstIndexLocation = drawIndexedAttribs.FirstIndexLocation;
        attribs.FirstInstanceLocation = drawIndexedAttribs.FirstInstanceLocation;
        attribs.NumInstances = drawIndexedAttribs.NumInstances;

        m_pImmediateContext->DrawIndexed(attribs);
    }

    void DiligentRendererContext::DispatchCompute(const DispatchComputeAttribs& dispatchComputeAttribs) {
        Diligent::DispatchComputeAttribs attribs;
        attribs.ThreadGroupCountX = dispatchComputeAttribs.ThreadGroupCountX;
        attribs.ThreadGroupCountY = dispatchComputeAttribs.ThreadGroupCountY;
        attribs.ThreadGroupCountZ = dispatchComputeAttribs.ThreadGroupCountZ;

        attribs.MtlThreadGroupSizeX = dispatchComputeAttribs.MtlThreadGroupSizeX;
        attribs.MtlThreadGroupSizeY = dispatchComputeAttribs.MtlThreadGroupSizeY;
        attribs.MtlThreadGroupSizeZ = dispatchComputeAttribs.MtlThreadGroupSizeZ;

        m_pImmediateContext->DispatchCompute(attribs);
    }

    ShaderHandle DiligentRendererContext::CreateShader(const ShaderCreateInfo& createInfo) {
        const Handle<Shader> handle = m_ShaderTable.Emplace();
        QShaderData* slot = m_ShaderTable.At(handle);

        QS_CORE_ASSERT(slot);

        // Own data
        slot->Name = createInfo.Specification.Name;
        slot->EntryPoint = createInfo.Specification.EntryPoint;

        ShaderSpec& shaderSpec = slot->Specification;
        shaderSpec.Name = slot->Name;
        shaderSpec.EntryPoint = slot->EntryPoint;
        shaderSpec.Type = createInfo.Specification.Type;

        Diligent::ShaderCreateInfo shaderCreateInfo;
        // Tell the system that the shader source code is in HLSL.
        // For OpenGL, the engine will convert this into GLSL behind the scene
        switch (m_RendererAPI) {
        case RendererAPI::Direct3D11:
        case RendererAPI::Direct3D12:
            shaderCreateInfo.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;
            shaderCreateInfo.Source = reinterpret_cast<const char*>(createInfo.ByteCode.data());
            shaderCreateInfo.SourceLength = createInfo.ByteCode.size();
            break;
        case RendererAPI::Vulkan:
            shaderCreateInfo.SourceLanguage = SHADER_SOURCE_LANGUAGE_BYTECODE;
            shaderCreateInfo.ByteCode = createInfo.ByteCode.data();
            shaderCreateInfo.ByteCodeSize = createInfo.ByteCode.size();
            break;
        case RendererAPI::OpenGL:
            shaderCreateInfo.SourceLanguage = SHADER_SOURCE_LANGUAGE_GLSL;
            shaderCreateInfo.Source = reinterpret_cast<const char*>(createInfo.ByteCode.data());
            shaderCreateInfo.SourceLength = createInfo.ByteCode.size();
            break;
        default:
            QS_CORE_ASSERT(false, "Unsupported renderer API");
            break;
        }

        // OpenGL backend requires emulated combined HLSL texture samplers (g_Texture + g_Texture_sampler combination)
        shaderCreateInfo.Desc.UseCombinedTextureSamplers = true;
        // Create shader
        shaderCreateInfo.Desc.ShaderType = Utils::GetShaderType(shaderSpec.Type);
        shaderCreateInfo.EntryPoint = slot->EntryPoint.c_str();
        shaderCreateInfo.Desc.Name = slot->Name.c_str();
        m_pDevice->CreateShader(shaderCreateInfo, &slot->Shader);

        if (!slot->Shader) [[unlikely]] {
            QS_CORE_ERROR_TAG(
                "DiligentRendererContext",
                "Failed to create shader '{}'!",
                createInfo.Specification.Name
            );

            m_ShaderTable.Erase(handle);
            return {};
        }

        return handle;
    }

    void DiligentRendererContext::Destroy(const ShaderHandle shaderHandle) {
        QShaderData* slot = m_ShaderTable.At(shaderHandle);
        if (!slot) [[unlikely]] {
            QS_CORE_ERROR_TAG(
                "DiligentRendererContext",
                "Destroy(ShaderHandle): Failed to find slot for shader handle ({},{})",
                shaderHandle.Index(),
                shaderHandle.Generation()
            );

            return;
        }

        slot->Shader->Release();
        slot->Name.clear();
        slot->EntryPoint.clear();
        slot->Specification = {};

        m_ShaderTable.Erase(shaderHandle);
    }

    PipelineStateHandle DiligentRendererContext::CreatePipelineState(
        const GraphicsPipelineStateCreateInfo& pipelineStateCreateInfo
    ) {
        Handle<PipelineStateObject> handle = m_PipelineStateTable.Emplace();
        PipelineStateData* slot = m_PipelineStateTable.At(handle);

        // Own data
        slot->Name = pipelineStateCreateInfo.Name;

        for (const ShaderResourceVariableSpec& variable : pipelineStateCreateInfo.Spec.ResourceLayout.Variables) {
            ShaderResourceVariableSpec spec = variable;

            slot->OwnedStrings.emplace_back(spec.Name);
            spec.Name = slot->OwnedStrings.back();

            slot->Variables.push_back(spec);
        }

        for (const ImmutableSamplerSpec& immutableSampler : pipelineStateCreateInfo.Spec.ResourceLayout.
             ImmutableSamplers) {
            ImmutableSamplerSpec spec = immutableSampler;

            slot->OwnedStrings.emplace_back(immutableSampler.SamplerOrTextureName);
            spec.SamplerOrTextureName = slot->OwnedStrings.back();

            slot->ImmutableSamplers.push_back(spec);
        }

        slot->PipelineSpec.ResourceLayout.Variables = slot->Variables;
        slot->PipelineSpec.ResourceLayout.ImmutableSamplers = slot->ImmutableSamplers;
        slot->PipelineSpec.Type = pipelineStateCreateInfo.Spec.Type;

        slot->GraphicsPipeline = pipelineStateCreateInfo.GraphicsPipeline;


        // Setup create info
        GraphicsPipelineSpec& gpSpec = slot->GraphicsPipeline;

        // Pipeline state object encompasses configuration of all GPU stages
        Diligent::GraphicsPipelineStateCreateInfo PSOCreateInfo;
        PipelineStateDesc& PSODesc = PSOCreateInfo.PSODesc;

        // Pipeline state name is used by the engine to report issues
        // It is always a good idea to give objects descriptive names
        PSODesc.Name = slot->Name.c_str();

        // This is a graphics pipeline
        PSODesc.PipelineType = PIPELINE_TYPE_GRAPHICS;

        // Set render target format which is the format of the swap chain's color buffer
        // Primitive topology defines what kind of primitives will be rendered by this pipeline state
        PSOCreateInfo.GraphicsPipeline.PrimitiveTopology = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        // No back face culling for this tutorial

        PSOCreateInfo.GraphicsPipeline.SmplDesc.Count = Utils::GetSampleCount(gpSpec.SampleSpec.Count);
        PSOCreateInfo.GraphicsPipeline.SmplDesc.Quality = gpSpec.SampleSpec.Quality;

        RasterizerStateDesc& rasterizerDesc = PSOCreateInfo.GraphicsPipeline.RasterizerDesc;
        rasterizerDesc.FillMode = Utils::GetFillMode(gpSpec.RasterizerSpec.FillMode);
        rasterizerDesc.CullMode = Utils::GetCullMode(gpSpec.RasterizerSpec.CullMode);
        rasterizerDesc.FrontCounterClockwise = gpSpec.RasterizerSpec.FrontCounterClockwise;
        rasterizerDesc.DepthClipEnable = gpSpec.RasterizerSpec.DepthClipEnable;
        rasterizerDesc.ScissorEnable = gpSpec.RasterizerSpec.ScissorEnabled;
        rasterizerDesc.AntialiasedLineEnable = gpSpec.RasterizerSpec.AntialiasedLineEnabled;
        rasterizerDesc.DepthBias = gpSpec.RasterizerSpec.DepthBias;
        rasterizerDesc.DepthBiasClamp = gpSpec.RasterizerSpec.DepthBiasClamp;
        rasterizerDesc.SlopeScaledDepthBias = gpSpec.RasterizerSpec.SlopeScaledDepthBias;

        DepthStencilStateDesc& depthStencilDesc = PSOCreateInfo.GraphicsPipeline.DepthStencilDesc;
        depthStencilDesc.DepthEnable = gpSpec.DepthStencilSpec.DepthEnable;
        depthStencilDesc.DepthWriteEnable = gpSpec.DepthStencilSpec.DepthWriteEnable;
        depthStencilDesc.DepthFunc = Utils::GetComparisonFunc(gpSpec.DepthStencilSpec.DepthFunc);
        depthStencilDesc.StencilEnable = gpSpec.DepthStencilSpec.StencilEnable;
        depthStencilDesc.StencilReadMask = gpSpec.DepthStencilSpec.StencilReadMask;
        depthStencilDesc.StencilWriteMask = gpSpec.DepthStencilSpec.StencilWriteMask;
        depthStencilDesc.FrontFace = Utils::GetStencilOp(gpSpec.DepthStencilSpec.FrontFace);
        depthStencilDesc.BackFace = Utils::GetStencilOp(gpSpec.DepthStencilSpec.BackFace);

        PSOCreateInfo.GraphicsPipeline.NumRenderTargets = gpSpec.NumRenderTargets;

        PSOCreateInfo.GraphicsPipeline.pRenderPass = m_RenderPassTable.At(gpSpec.RenderPass)->RenderPass;

        PSOCreateInfo.GraphicsPipeline.BlendDesc = Utils::GetBlendStateSpec(gpSpec.BlendSpec);

        Utils::DiligentInputLayoutDesc inputLayoutDesc(gpSpec.InputLayout);

        PSOCreateInfo.GraphicsPipeline.InputLayout = inputLayoutDesc.Desc;

        // Finally, create the pipeline state
        if (pipelineStateCreateInfo.VertexShader.IsValid()) {
            PSOCreateInfo.pVS = m_ShaderTable.At(pipelineStateCreateInfo.VertexShader)->Shader;
        }
        if (pipelineStateCreateInfo.FragmentShader.IsValid()) {
            PSOCreateInfo.pPS = m_ShaderTable.At(pipelineStateCreateInfo.FragmentShader)->Shader;
        }

        // Define variable type that will be used by default
        Vec<ShaderResourceVariableDesc> variables;
        variables.resize(slot->Variables.size());
        for (uint32_t i = 0; i < variables.size(); i++) {
            const ShaderResourceVariableSpec& variable = slot->Variables[i];
            ShaderResourceVariableDesc& desc = variables[i];

            desc.Name = variable.Name.data();
            desc.Type = Utils::GetShaderResourceVariableType(variable.Type);
            desc.ShaderStages = Utils::GetShaderType(variable.ShaderStages);
            desc.Flags = Utils::GetShaderVariableFlags(variable.Flags);
        }

        PSOCreateInfo.PSODesc.ResourceLayout.Variables = variables.data();
        PSOCreateInfo.PSODesc.ResourceLayout.NumVariables = variables.size();

        Vec<ImmutableSamplerDesc> immutableSamplers;
        immutableSamplers.resize(slot->ImmutableSamplers.size());
        for (uint32_t i = 0; i < immutableSamplers.size(); i++) {
            const ImmutableSamplerSpec& immutableSampler = slot->ImmutableSamplers[i];
            ImmutableSamplerDesc& desc = immutableSamplers[i];

            desc.ShaderStages = Utils::GetShaderType(immutableSampler.ShaderStages);
            desc.Desc = Utils::GetSamplerDesc(immutableSampler.Specification);
            desc.SamplerOrTextureName = immutableSampler.SamplerOrTextureName.data();
        }

        PSOCreateInfo.PSODesc.ResourceLayout.ImmutableSamplers = immutableSamplers.data();
        PSOCreateInfo.PSODesc.ResourceLayout.NumImmutableSamplers = immutableSamplers.size();

        m_pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &slot->PSO);

        if (!slot->PSO) {
            QS_CORE_ERROR_TAG(
                "DiligentRendererContext",
                "CreatePipelineState(GraphicsPipelineStateCreateInfo): Failed to create graphics pipeline state object"
            );

            m_PipelineStateTable.Erase(handle);
            return {};
        }

        return handle;
    }

    PipelineStateHandle DiligentRendererContext::CreatePipelineState(
        const ComputePipelineStateCreateInfo& pipelineStateCreateInfo
    ) {
        Handle<PipelineStateObject> handle = m_PipelineStateTable.Emplace();
        PipelineStateData* slot = m_PipelineStateTable.At(handle);

        // Own data
        for (const ShaderResourceVariableSpec& variable : pipelineStateCreateInfo.Spec.ResourceLayout.Variables) {
            ShaderResourceVariableSpec spec = variable;

            slot->OwnedStrings.emplace_back(spec.Name);
            spec.Name = slot->OwnedStrings.back();

            slot->Variables.push_back(spec);
        }

        for (const ImmutableSamplerSpec& immutableSampler : pipelineStateCreateInfo.Spec.ResourceLayout.
             ImmutableSamplers) {
            ImmutableSamplerSpec spec = immutableSampler;

            slot->OwnedStrings.emplace_back(immutableSampler.SamplerOrTextureName);
            spec.SamplerOrTextureName = slot->OwnedStrings.back();

            slot->ImmutableSamplers.push_back(spec);
        }

        slot->PipelineSpec.ResourceLayout.Variables = slot->Variables;
        slot->PipelineSpec.ResourceLayout.ImmutableSamplers = slot->ImmutableSamplers;
        slot->PipelineSpec.Type = pipelineStateCreateInfo.Spec.Type;

        // Pipeline state object encompasses configuration of all GPU stages
        Diligent::ComputePipelineStateCreateInfo PSOCreateInfo;
        PipelineStateDesc& PSODesc = PSOCreateInfo.PSODesc;

        // This is a graphics pipeline
        PSODesc.PipelineType = PIPELINE_TYPE_COMPUTE;

        PSOCreateInfo.pCS = m_ShaderTable.At(pipelineStateCreateInfo.ComputeShader)->Shader;

        // Define variable type that will be used by default
        Vec<ShaderResourceVariableDesc> variables;
        variables.resize(slot->Variables.size());
        for (uint32_t i = 0; i < variables.size(); i++) {
            const ShaderResourceVariableSpec& variable = slot->Variables[i];
            ShaderResourceVariableDesc& desc = variables[i];

            desc.Name = variable.Name.data();
            desc.Type = Utils::GetShaderResourceVariableType(variable.Type);
            desc.ShaderStages = Utils::GetShaderType(variable.ShaderStages);
            desc.Flags = Utils::GetShaderVariableFlags(variable.Flags);
        }

        PSOCreateInfo.PSODesc.ResourceLayout.Variables = variables.data();
        PSOCreateInfo.PSODesc.ResourceLayout.NumVariables = variables.size();

        Vec<ImmutableSamplerDesc> immutableSamplers;
        immutableSamplers.resize(slot->ImmutableSamplers.size());
        for (uint32_t i = 0; i < immutableSamplers.size(); i++) {
            const ImmutableSamplerSpec& immutableSampler = slot->ImmutableSamplers[i];
            ImmutableSamplerDesc& desc = immutableSamplers[i];

            desc.ShaderStages = Utils::GetShaderType(immutableSampler.ShaderStages);
            desc.Desc = Utils::GetSamplerDesc(immutableSampler.Specification);
            desc.SamplerOrTextureName = immutableSampler.SamplerOrTextureName.data();
        }

        PSOCreateInfo.PSODesc.ResourceLayout.ImmutableSamplers = immutableSamplers.data();
        PSOCreateInfo.PSODesc.ResourceLayout.NumImmutableSamplers = immutableSamplers.size();

        m_pDevice->CreateComputePipelineState(PSOCreateInfo, &slot->PSO);

        if (!slot->PSO) {
            QS_CORE_ERROR_TAG(
                "DiligentRendererContext",
                "CreatePipelineState(ComputePipelineStateCreateInfo): Failed to create compute pipeline state object!"
            );

            m_PipelineStateTable.Erase(handle);
            return {};
        }

        return handle;
    }

    PipelineResourceSignatureHandle DiligentRendererContext::CreatePipelineResourceSignature(
        const PipelineResourceSignatureSpec& pipelineResourceSignatureSpec
    ) {
        Handle<PipelineResourceSignature> handle = m_PipelineResourceSignatureTable.Emplace();
        PipelineResourceSignatureData* slot = m_PipelineResourceSignatureTable.At(handle);

        // Own data
        auto& spec = slot->Specification;

        slot->Name = pipelineResourceSignatureSpec.Name;

        for (const PipelineResourceSpec& resource : pipelineResourceSignatureSpec.Resources) {
            PipelineResourceSpec owned = resource;
            slot->OwnedStrings.emplace_back(resource.Name);
            owned.Name = slot->OwnedStrings.back();

            slot->Resources.push_back(owned);
        }
        spec.Resources = slot->Resources;

        for (const ImmutableSamplerSpec& immutableSampler : pipelineResourceSignatureSpec.ImmutableSamplers) {
            ImmutableSamplerSpec owned = immutableSampler;
            slot->OwnedStrings.emplace_back(immutableSampler.SamplerOrTextureName);
            owned.SamplerOrTextureName = slot->OwnedStrings.back();

            slot->ImmutableSamplers.push_back(owned);
        }
        spec.ImmutableSamplers = slot->ImmutableSamplers;

        slot->CombinedSamplerSuffix = pipelineResourceSignatureSpec.CombinedSamplerSuffix;
        spec.CombinedSamplerSuffix = slot->CombinedSamplerSuffix;

        spec.UseCombinedTextureSamplers = pipelineResourceSignatureSpec.UseCombinedTextureSamplers;
        spec.BindingIndex = pipelineResourceSignatureSpec.BindingIndex;
        spec.SRBAllocationGranularity = pipelineResourceSignatureSpec.SRBAllocationGranularity;

        // Create signature
        PipelineResourceSignatureDesc desc = Utils::GetPipelineResourceSignatureDesc(spec);
        m_pDevice->CreatePipelineResourceSignature(desc, &slot->Signature);

        if (!slot->Signature) {
            QS_CORE_ERROR_TAG(
                "DiligentRendererContext",
                "CreatePipelineResourceSignature(PipelineResourceSignatureSpec):"
                " Failed to create pipeline resource signature object (name: {})",
                spec.Name
            );

            m_PipelineResourceSignatureTable.Erase(handle);
            return {};
        }

        return handle;
    }

    void DiligentRendererContext::Destroy(PipelineResourceSignatureHandle pipelineResourceSignatureHandle) {
        PipelineResourceSignatureData* slot = m_PipelineResourceSignatureTable.At(pipelineResourceSignatureHandle);
        if (!slot) {
            QS_CORE_ERROR_TAG(
                "DiligentRendererContex",
                "Failed to find PipelineResourceSignatureData at slot ({},{})",
                pipelineResourceSignatureHandle.Index(), pipelineResourceSignatureHandle.Generation()
            );

            return;
        }

        slot->Signature->Release();

        m_PipelineResourceSignatureTable.Erase(pipelineResourceSignatureHandle);
    }

    void DiligentRendererContext::BindStaticVariableByName(
        const PipelineStateHandle pipelineStateHandle,
        const ShaderType shaderType,
        std::string_view name,
        const GpuBufferHandle gpuBufferHandle
    ) {
        IPipelineState* pso = m_PipelineStateTable.At(pipelineStateHandle)->PSO;

        QS_CORE_ASSERT(pso, "Pipeline state object is nullptr!");

        IShaderResourceVariable* resourceVariable = pso->GetStaticVariableByName(
            Utils::GetShaderType(shaderType),
            FormatTemp("{}", name)
        );

        if (resourceVariable) {
            resourceVariable->Set(m_BufferTable.At(gpuBufferHandle)->Buffer, SET_SHADER_RESOURCE_FLAG_ALLOW_OVERWRITE);
        }
    }

    void DiligentRendererContext::BindStaticVariableByName(
        const PipelineStateHandle pipelineStateHandle,
        const ShaderType shaderType,
        std::string_view name,
        const GpuBufferViewHandle gpuBufferViewHandle
    ) {
        IPipelineState* pso = m_PipelineStateTable.At(pipelineStateHandle)->PSO;

        QS_CORE_ASSERT(pso, "Pipeline state object is nullptr!");

        IShaderResourceVariable* resourceVariable = pso->GetStaticVariableByName(
            Utils::GetShaderType(shaderType),
            FormatTemp("{}", name)
        );

        if (resourceVariable) {
            resourceVariable->Set(
                m_BufferViewTable.At(gpuBufferViewHandle)->BufferView,
                SET_SHADER_RESOURCE_FLAG_ALLOW_OVERWRITE
            );
        }
    }

    void DiligentRendererContext::BindPipelineState(const PipelineStateHandle pipelineStateHandle) {
        PipelineStateData* slot = m_PipelineStateTable.At(pipelineStateHandle);
        if (!slot) [[unlikely]] {
            QS_CORE_ERROR_TAG(
                "DiligentRendererContext",
                "BindPipelineState(PipelineStateHandle): Failed to get slot for pipeline state handle ({},{})",
                pipelineStateHandle.Index(),
                pipelineStateHandle.Generation()
            );

            return;
        }

        m_pImmediateContext->SetPipelineState(slot->PSO);
    }

    void DiligentRendererContext::Destroy(const PipelineStateHandle pipelineStateHandle) {
        PipelineStateData* slot = m_PipelineStateTable.At(pipelineStateHandle);
        if (!slot) [[unlikely]] {
            QS_CORE_ERROR_TAG(
                "DiligentRendererContext",
                "Destroy(PipelineStateHandle): Failed to get slot for pipeline state handle ({},{})",
                pipelineStateHandle.Index(),
                pipelineStateHandle.Generation()
            );

            return;
        }

        slot->PSO->Release();
        slot->PSO = nullptr;
        slot->GraphicsPipeline = {};
        slot->PipelineSpec = {};
        slot->Name.clear();
        slot->Variables.clear();
        slot->ImmutableSamplers.clear();
        slot->OwnedStrings.clear();

        m_PipelineStateTable.Erase(pipelineStateHandle);
    }

    FenceHandle DiligentRendererContext::CreateFence(const FenceSpec& fenceSpec) {
        const FenceHandle handle = m_FenceTable.Emplace();
        QS_CORE_ASSERT(
            handle,
            "DiligentRendererContext::CreateFence(FenceSpec): Failed to emplace fence handle!"
        );

        FenceData* slot = m_FenceTable.At(handle);

        slot->Name = fenceSpec.Name;
        slot->Spec = fenceSpec;

        FenceDesc desc;
        desc.Name = slot->Name.c_str();
        desc.Type = Utils::GetFenceType(slot->Spec.Type);

        m_pDevice->CreateFence(desc, &slot->Fence);

        QS_CORE_ASSERT(
            slot->Fence,
            "DiligentRendererContext::CreateFence(FenceSpec): Failed to create fence!"
        );

        return handle;
    }

    void DiligentRendererContext::Destroy(const FenceHandle fenceHandle) {
        FenceData* slot = m_FenceTable.At(fenceHandle);
        QS_CORE_ASSERT(
            slot,
            "DiligentRendererContext::Destroy(FenceHandle) failed to get slot at handle ({},{})!",
            fenceHandle.Index(), fenceHandle.Generation()
        );

        slot->Fence->Release();
        slot->Fence = nullptr;

        m_FenceTable.Erase(fenceHandle);
    }

    void DiligentRendererContext::EnqueueSignal(const FenceHandle fenceHandle, const uint64_t value) {
        m_pImmediateContext->EnqueueSignal(m_FenceTable.At(fenceHandle)->Fence, value);
    }

    uint64_t DiligentRendererContext::FenceGetCompletedValue(const FenceHandle fenceHandle) {
        return m_FenceTable.At(fenceHandle)->Fence->GetCompletedValue();
    }

    void DiligentRendererContext::WaitForFence(const FenceHandle fenceHandle, const uint64_t value) {
        m_pImmediateContext->DeviceWaitForFence(m_FenceTable.At(fenceHandle)->Fence, value);
    }

    VertexBufferHandle
    DiligentRendererContext::CreateVertexBuffer(const BufferView vertices, VertexLayout bufferLayout) {
        BufferDesc vertBuffDesc;
        vertBuffDesc.Name = "VertexBuffer";
        vertBuffDesc.Usage = USAGE_IMMUTABLE;
        vertBuffDesc.BindFlags = BIND_VERTEX_BUFFER;
        vertBuffDesc.Size = vertices.size();

        BufferData vbData;
        vbData.pData = vertices.data();
        vbData.DataSize = vertices.size();

        const Handle<VertexBuffer> handle = m_VertexBufferTable.Emplace();
        IBuffer** slot = m_VertexBufferTable.At(handle);

        m_pDevice->CreateBuffer(vertBuffDesc, &vbData, slot);

        StateTransitionDesc barriers[] = {
            {*slot, RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_VERTEX_BUFFER, STATE_TRANSITION_FLAG_UPDATE_STATE},
        };

        m_pImmediateContext->TransitionResourceStates(1, barriers);

        return handle;
    }

    void DiligentRendererContext::BindVertexBuffer(VertexBufferHandle vertexBufferHandle, uint32_t stream) {
        IBuffer** slot = m_VertexBufferTable.At(vertexBufferHandle);
        if (!slot) {
            QS_CORE_ERROR_TAG(
                "RendererContext",
                "No vertex buffer found with handle ({},{})",
                vertexBufferHandle.Index(),
                vertexBufferHandle.Generation()
            );

            return;
        }

        const Uint64 offset = 0;
        m_pImmediateContext->SetVertexBuffers(
            stream,
            1,
            slot,
            &offset,
            RESOURCE_STATE_TRANSITION_MODE_NONE,
            SET_VERTEX_BUFFERS_FLAG_RESET
        );
    }

    IndexBufferHandle DiligentRendererContext::CreateIndexBuffer(const Span<uint16_t> indices) {
        BufferView data = std::as_bytes(indices);
        BufferDesc indBuffDesc;
        indBuffDesc.Name = "IndexBuffer";
        indBuffDesc.Usage = USAGE_IMMUTABLE;
        indBuffDesc.BindFlags = BIND_INDEX_BUFFER;
        indBuffDesc.Size = data.size();

        BufferData ibData;
        ibData.pData = data.data();
        ibData.DataSize = data.size();

        const Handle<IndexBuffer> handle = m_IndexBufferTable.Emplace();
        IBuffer** slot = m_IndexBufferTable.At(handle);

        m_pDevice->CreateBuffer(indBuffDesc, &ibData, slot);

        StateTransitionDesc barriers[] = {
            {*slot, RESOURCE_STATE_UNKNOWN, RESOURCE_STATE_INDEX_BUFFER, STATE_TRANSITION_FLAG_UPDATE_STATE},
        };

        m_pImmediateContext->TransitionResourceStates(1, barriers);

        return handle;
    }

    void DiligentRendererContext::BindIndexBuffer(const IndexBufferHandle indexBufferHandle) {
        IBuffer** slot = m_IndexBufferTable.At(indexBufferHandle);
        if (!slot) [[unlikely]] {
            QS_CORE_ERROR_TAG(
                "RendererContext",
                "No vertex buffer found with handle ({},{})",
                indexBufferHandle.Index(),
                indexBufferHandle.Generation()
            );

            return;
        }

        m_pImmediateContext->SetIndexBuffer(*slot, 0, RESOURCE_STATE_TRANSITION_MODE_NONE);
    }

    void DiligentRendererContext::Destroy(const IndexBufferHandle indexBufferHandle) {
        IBuffer** slot = m_IndexBufferTable.At(indexBufferHandle);
        if (!slot) [[unlikely]] {
            QS_CORE_ERROR_TAG(
                "RendererContext",
                "No index buffer found with handle ({},{})",
                indexBufferHandle.Index(),
                indexBufferHandle.Generation()
            );

            return;
        }

        (*slot)->Release();
        *slot = nullptr;

        m_IndexBufferTable.Erase(indexBufferHandle);
    }

    void DiligentRendererContext::Shutdown() {
        m_pImmediateContext->Flush();

        m_pDevice.Release();
        m_pSwapChain.Release();
        m_pImmediateContext.Release();
    }

    void DiligentRendererContext::IncRef(Handle<Texture> textureHandle) { m_TextureTable.IncRef(textureHandle); }
    void DiligentRendererContext::DecRef(Handle<Texture> textureHandle) {
        if (m_TextureTable.DecRef(textureHandle)) {
            Destroy(textureHandle);
        }
    }
    void DiligentRendererContext::IncRef(Handle<FrameBuffer> frameBuffer) { m_FrameBufferTable.IncRef(frameBuffer); }
    void DiligentRendererContext::DecRef(Handle<FrameBuffer> frameBuffer) {
        if (m_FrameBufferTable.DecRef(frameBuffer)) {
            Destroy(frameBuffer);
        }
    }

    void DiligentRendererContext::IncRef(Handle<IndexBuffer> indexBuffer) { m_IndexBufferTable.IncRef(indexBuffer); }
    void DiligentRendererContext::DecRef(Handle<IndexBuffer> indexBuffer) {
        if (m_IndexBufferTable.DecRef(indexBuffer)) {
            Destroy(indexBuffer);
        }
    }

    void DiligentRendererContext::IncRef(Handle<VertexBuffer> vertexBuffer) {m_VertexBufferTable.IncRef(vertexBuffer);}
    void DiligentRendererContext::DecRef(Handle<VertexBuffer> vertexBuffer) {
        if (m_VertexBufferTable.DecRef(vertexBuffer)) {
            Destroy(vertexBuffer);
        }
    }

    void DiligentRendererContext::IncRef(Handle<GpuBuffer> gpuBuffer) {m_BufferTable.IncRef(gpuBuffer);}
    void DiligentRendererContext::DecRef(Handle<GpuBuffer> gpuBuffer) {
        if (m_BufferTable.DecRef(gpuBuffer)) {
            Destroy(gpuBuffer);
        }
    }

    void DiligentRendererContext::IncRef(Handle<RenderPass> renderPass) {m_RenderPassTable.IncRef(renderPass);}
    void DiligentRendererContext::DecRef(Handle<RenderPass> renderPass) {
        if (m_RenderPassTable.DecRef(renderPass)) {
            Destroy(renderPass);
        }
    }

    void DiligentRendererContext::IncRef(Handle<Fence> fence) { m_FenceTable.IncRef(fence); }
    void DiligentRendererContext::DecRef(Handle<Fence> fence) {
        if (m_FenceTable.DecRef(fence)) {
            Destroy(fence);
        }
    }

    void DiligentRendererContext::IncRef(Handle<ShaderResourceBinding> srb) {m_ShaderResourceBindingTable.IncRef(srb);}
    void DiligentRendererContext::DecRef(Handle<ShaderResourceBinding> srb) {
        if (m_ShaderResourceBindingTable.DecRef(srb)) {
            Destroy(srb);
        }
    }

    void DiligentRendererContext::IncRef(Handle<PipelineStateObject> pso) {m_PipelineStateTable.IncRef(pso);}
    void DiligentRendererContext::DecRef(Handle<PipelineStateObject> pso) {
        if (m_PipelineStateTable.DecRef(pso)) {
            Destroy(pso);
        }
    }

    bool DiligentRendererContext::IsAlive(PipelineStateHandle pipelineStateHandle) {
        return m_PipelineStateTable.IsAlive(pipelineStateHandle);
    }
}
