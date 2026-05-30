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
            IFramebuffer*& frameBuffer, IRenderDevice* device,
            const Span32<ITexture*> attachments, IRenderPass* renderPass
        ) {
            SmallVec<ITextureView*, 2> viewAttachments;
            viewAttachments.reserve(attachments.size());
            for (uint64_t i = 0; i < attachments.size() - 1; i++) {
                viewAttachments.push_back(attachments[i]->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET));
            }

            viewAttachments.push_back(
                attachments[attachments.size() - 1]->GetDefaultView(TEXTURE_VIEW_DEPTH_STENCIL)
            );

            FramebufferDesc desc;
            desc.Name = "FrameBuffer";
            desc.ppAttachments = viewAttachments.data();
            desc.AttachmentCount = attachments.size();
            if (renderPass) {
                desc.pRenderPass = renderPass;
            }
            desc.Width = attachments[0]->GetDesc().GetWidth();
            desc.Height = attachments[0]->GetDesc().GetHeight();
            desc.NumArraySlices = 1;

            device->CreateFramebuffer(
                desc,
                &frameBuffer
            );
        }

        static TEXTURE_FORMAT GetFormat(const ImageFormat imageFormat) {
            switch (imageFormat) {
            case ImageFormat::None:
                return TEX_FORMAT_UNKNOWN;
            case ImageFormat::RED8UNORM:
                return TEX_FORMAT_R8_UNORM;
            case ImageFormat::RED8UINT:
                return TEX_FORMAT_R8_UINT;
            case ImageFormat::RED16UINT:
                return TEX_FORMAT_R16_UINT;
            case ImageFormat::RED32UINT:
                return TEX_FORMAT_R32_UINT;
            case ImageFormat::RED32FLOAT:
                return TEX_FORMAT_R32_FLOAT;
            case ImageFormat::RG8UNORM:
                return TEX_FORMAT_RG8_UNORM;
            case ImageFormat::RG16FLOAT:
                return TEX_FORMAT_RG16_FLOAT;
            case ImageFormat::RG32FLOAT:
                return TEX_FORMAT_RG32_FLOAT;
            case ImageFormat::RGB:
                return TEX_FORMAT_ETC2_RGB8_UNORM;
            case ImageFormat::RGBA:
                return TEX_FORMAT_RGBA8_UNORM;
            case ImageFormat::RGBA16FLOAT:
                return TEX_FORMAT_RGBA16_FLOAT;
            case ImageFormat::RGBA32FLOAT:
                return TEX_FORMAT_RGBA32_FLOAT;
            case ImageFormat::B10R11G11FLOAT:
                return TEX_FORMAT_R11G11B10_FLOAT;
            case ImageFormat::SRGB:
                return TEX_FORMAT_BC7_UNORM_SRGB;
            case ImageFormat::SRGBA:
                return TEX_FORMAT_RGBA8_UNORM_SRGB;
            case ImageFormat::DEPTH32FSTENCIL8UINT:
                return TEX_FORMAT_D32_FLOAT_S8X24_UINT;
            case ImageFormat::DEPTH32F:
                return TEX_FORMAT_D32_FLOAT;
            case ImageFormat::DEPTH24STENCIL8:
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

        constexpr USAGE GetBufferUsage(Usage usage) {
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
    }

    namespace TextureUtil {

        static RESOURCE_DIMENSION GetTextureType(const TextureType textureType) {
            switch (textureType) {
            case TextureType::Texture2D:
                return RESOURCE_DIM_TEX_2D;
            case TextureType::TextureCube:
                return RESOURCE_DIM_TEX_CUBE;
            }

            return RESOURCE_DIM_UNDEFINED;
        }

        static void CreateTexture(IRenderDevice* device, QTextureData& textureData) {
            const TextureSpecification& spec = textureData.Specification;

            TextureDesc textureDesc;
            textureDesc.Name = spec.Name.c_str();
            textureDesc.Format = Utils::GetFormat(spec.Format);
            textureDesc.Width = spec.Width;
            textureDesc.Height = spec.Height;
            textureDesc.Type = GetTextureType(spec.Type);
            textureDesc.BindFlags = Utils::GetBindFlags(spec.BindFlags);
            textureDesc.SampleCount = Utils::GetSampleCount(spec.SampleCount);

            device->CreateTexture(textureDesc, nullptr, &textureData.Texture);
        }

        void Resize(
            IRenderDevice* device,
            QTextureData& data,
            const uint32_t width,
            const uint32_t height
        ) {
            data.Texture->Release();
            data.Texture = nullptr;

            data.Specification.Width = width;
            data.Specification.Height = height;

            CreateTexture(device, data);
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

#ifndef QS_PLATFORM_MACOS
            VkPhysicalDeviceVulkan12Features vk12Features{};
            vk12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
            vk12Features.drawIndirectCount = VK_TRUE;
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
        m_pSwapChain->Present(0);
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

    GraphicsShaderHandle DiligentRendererContext::CreateShader(
        Buffer vertex, Buffer fragment, const std::string& name
    ) {
        // TODO:
        return {};
    }

    bool DiligentRendererContext::RecreateShader(GraphicsShaderHandle handle, Buffer vertex, Buffer fragment) {
        // TODO:
        return {};
    }

    void DiligentRendererContext::Destroy(GraphicsShaderHandle shaderHandle) {
        // TODO:
    }

    GpuBufferHandle DiligentRendererContext::CreateBuffer(const GPUBufferSpec& bufferSpec, const BufferView data) {
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
        desc.Usage = Utils::GetBufferUsage(bufferSpec.Usage);
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
        GpuBufferHandle gpuBufferHandle
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

        shaderResourceVariable->Set(m_BufferTable.At(gpuBufferHandle)->Buffer->GetDefaultView(BUFFER_VIEW_SHADER_RESOURCE));
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

    void DiligentRendererContext::CommitShaderResources(const ShaderResourceBindingHandle shaderResourceBindingHandle) {
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
            RESOURCE_STATE_TRANSITION_MODE_NONE
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
        slot->Buffer->Release();
        slot->Buffer = nullptr;
        slot->Specification = {};
        slot->Name.clear();

        m_BufferTable.Erase(bufferHandle);
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
        TextureUtil::CreateTexture(m_pDevice, *slot);

        return handle;
    }

    TextureHandle DiligentRendererContext::CreateTexture(const TextureSpecification& spec, Buffer data) {
        return CreateTexture(spec);
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
        TextureUtil::Resize(m_pDevice, *m_TextureTable.At(textureHandle), width, height);
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
            if (subPass.ResolveAttachments) {
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

            if (subPass.ResolveAttachments) {
                const uint32_t resolveAttachmentIndex = slot->AttachmentReferences.size();
                for (uint32_t i = 0; i < subPass.RenderTargetAttachments.size(); i++) {
                    slot->AttachmentReferences.push_back(subPass.ResolveAttachments[i]);
                }

                ownedSubpass.ResolveAttachments = &slot->AttachmentReferences[resolveAttachmentIndex];
            }

            ownedSubpass.DepthAttachment = subPass.DepthAttachment;

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
            if (pass.ResolveAttachments) {
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

            const uint32_t resolveAttachmentIndex = attachmentRefs.size();
            for (uint32_t i = 0; i < pass.RenderTargetAttachments.size(); i++) {
                attachmentRefs.push_back(Utils::GetAttachmentReference(pass.ResolveAttachments[i]));
            }

            subPassDesc.pResolveAttachments = attachmentRefs.data() + resolveAttachmentIndex;

            attachmentRefs.push_back(Utils::GetAttachmentReference(pass.DepthAttachment));
            subPassDesc.pDepthStencilAttachment = &attachmentRefs.back();
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
        slot->Attachments = SmallVec<TextureHandle, 2>(frameBufferSpec.Attachments);

        FrameBufferSpec& spec = slot->Specification;
        spec = {slot->Name, slot->Attachments, frameBufferSpec.RenderPassHandle, frameBufferSpec.Size};

        SmallVec<ITexture*, 2> textureAttachments;
        textureAttachments.reserve(spec.Attachments.size());
        for (const TextureHandle Attachment : spec.Attachments) {
            textureAttachments.push_back(m_TextureTable.At(Attachment)->Texture);
        }

        auto& textureDesc = textureAttachments[0]->GetDesc();
        QS_CORE_ASSERT(
            spec.Size.Width == textureDesc.Width && spec.Size.Height == textureDesc.Height,
            "Frame buffer size does not match texture size!"
        );

        Utils::CreateFrameBuffer(
            slot->FrameBuffer,
            m_pDevice,
            Span32(textureAttachments),
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

        SmallVec<ITexture*, 2> textureAttachments;
        for (const TextureHandle attachment : data->Attachments) {
            QTextureData* texture = m_TextureTable.At(attachment);
            TextureUtil::Resize(m_pDevice, *texture, width, height);
            textureAttachments.push_back(texture->Texture);
        }

        data->FrameBuffer->Release();
        data->FrameBuffer = nullptr;

        IRenderPass* renderPass = nullptr;
        if (data->Specification.RenderPassHandle.IsValid()) {
            renderPass = m_RenderPassTable.At(data->Specification.RenderPassHandle)->RenderPass;
        }

        Utils::CreateFrameBuffer(data->FrameBuffer, m_pDevice, Span32(textureAttachments), renderPass);
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

    ShaderHandle DiligentRendererContext::CreateShader(const ShaderCreateInfo& createInfo) {
        const Handle<Shader> handle = m_ShaderTable.Emplace();
        ShaderData* slot = m_ShaderTable.At(handle);

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
        shaderCreateInfo.SourceLanguage = SHADER_SOURCE_LANGUAGE_BYTECODE;
        switch (m_RendererAPI) {
        case RendererAPI::Direct3D11:
        case RendererAPI::Direct3D12:
        case RendererAPI::Vulkan:
            shaderCreateInfo.SourceLanguage = SHADER_SOURCE_LANGUAGE_BYTECODE;
            shaderCreateInfo.ByteCode = createInfo.ByteCode.data();
            shaderCreateInfo.ByteCodeSize = createInfo.ByteCode.size();
            break;
        case RendererAPI::OpenGL:
            shaderCreateInfo.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;
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
        ShaderData* slot = m_ShaderTable.At(shaderHandle);
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
        PSOCreateInfo.GraphicsPipeline.RasterizerDesc.CullMode = Utils::GetCullMode(
            gpSpec.RasterizerSpec.CullMode
        );

        PSOCreateInfo.GraphicsPipeline.SmplDesc.Count = Utils::GetSampleCount(gpSpec.SampleSpec.Count);
        PSOCreateInfo.GraphicsPipeline.SmplDesc.Quality = gpSpec.SampleSpec.Quality;

        PSOCreateInfo.GraphicsPipeline.RasterizerDesc.FrontCounterClockwise = gpSpec.RasterizerSpec.
            FrontCounterClockwise;
        // Disable depth testing
        PSOCreateInfo.GraphicsPipeline.DepthStencilDesc.DepthEnable = gpSpec.DepthStencilSpec.DepthEnable;
        PSOCreateInfo.GraphicsPipeline.pRenderPass = m_RenderPassTable.At(gpSpec.RenderPass)->RenderPass;

        Utils::DiligentInputLayoutDesc inputLayoutDesc(gpSpec.InputLayout);

        /*
        Diligent::LayoutElement LayoutElems[] = {
            // Attribute 0 - vertex position
            {0, 0, 3, VT_FLOAT32, False},
            // Attribute 1 - vertex normal
            {1, 0, 3, VT_FLOAT32, False},
            // Attribute 2 - vertex tangent
            {2, 0, 3, VT_FLOAT32, False},
            // Attribute 3 - vertex uv
            {3, 0, 2, VT_FLOAT32, False}
        };

        PSOCreateInfo.GraphicsPipeline.InputLayout.LayoutElements = LayoutElems;
        PSOCreateInfo.GraphicsPipeline.InputLayout.NumElements = _countof(LayoutElems);
        */

        PSOCreateInfo.GraphicsPipeline.InputLayout = inputLayoutDesc.Desc;

        // Finally, create the pipeline state
        PSOCreateInfo.pVS = m_ShaderTable.At(pipelineStateCreateInfo.VertexShader)->Shader;
        PSOCreateInfo.pPS = m_ShaderTable.At(pipelineStateCreateInfo.FragmentShader)->Shader;

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
                "CreatePipelineState(PipelineStateSpec): Failed to create pipeline state object"
            );

            return {};
        }

        return handle;
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
            resourceVariable->Set(m_BufferTable.At(gpuBufferHandle)->Buffer);
        }
    }

    void DiligentRendererContext::BindPipelineState(PipelineStateHandle pipelineStateHandle) {
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

    void DiligentRendererContext::Destroy(PipelineStateHandle pipelineStateHandle) {
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

    void DiligentRendererContext::Submit(GraphicsShaderHandle shaderHandle, uint32_t view) {
        // TODO:
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
