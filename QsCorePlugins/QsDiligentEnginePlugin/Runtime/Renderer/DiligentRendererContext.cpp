#include "DiligentRendererContext.h"

#include "DataBlobImpl.hpp"
#include "DefaultRawMemoryAllocator.hpp"
#include "Texture.h"
#include "DataBlobImpl.hpp"

#if QS_PLATFORM_MACOS
#include "Quelos/Platform/MacOS/WindowHelper.h"
#endif

namespace Quelos {
    static const char* VSSource = R"(
struct PSInput
{
    float4 Pos   : SV_POSITION;
    float3 Color : COLOR;
};

void main(in  uint    VertId : SV_VertexID,
          out PSInput PSIn)
{
    float4 Pos[3];
    Pos[0] = float4(-0.5, -0.5, 0.0, 1.0);
    Pos[1] = float4( 0.0, +0.5, 0.0, 1.0);
    Pos[2] = float4(+0.5, -0.5, 0.0, 1.0);

    float3 Col[3];
    Col[0] = float3(1.0, 0.0, 0.0); // red
    Col[1] = float3(0.0, 1.0, 0.0); // green
    Col[2] = float3(0.0, 0.0, 1.0); // blue

    PSIn.Pos   = Pos[VertId];
    PSIn.Color = Col[VertId];
}
)";

    // Pixel shader will simply output interpolated vertex color
    static const char* PSSource = R"(
struct PSInput
{
    float4 Pos   : SV_POSITION;
    float3 Color : COLOR;
};

struct PSOutput
{
    float4 Color : SV_TARGET;
};

void main(in  PSInput  PSIn,
          out PSOutput PSOut)
{
    PSOut.Color = float4(PSIn.Color.rgb, 1.0);
}
)";

    DiligentRendererContext* DiligentRendererContext::s_Instance = nullptr;

    void DiligentRendererContext::Init(const Ref<Window>& window, RendererAPI api) {
        m_RendererAPI = api;
        m_Window = window;
        s_Instance = this;

        SwapChainDesc SCDesc;
        SCDesc.BufferCount = 3;
        SCDesc.Width = window->GetWidth();
        SCDesc.Height = window->GetHeight();

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
            const EngineVkCreateInfo engineCi;

            pFactoryVk->CreateDeviceAndContextsVk(engineCi, &m_pDevice, &m_pImmediateContext);
            pFactoryVk->CreateSwapChainVk(m_pDevice, m_pImmediateContext, SCDesc, nativeWindow, &m_pSwapChain);
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

        // Pipeline state object encompasses configuration of all GPU stages
        GraphicsPipelineStateCreateInfo PSOCreateInfo;
        PipelineStateDesc& PSODesc = PSOCreateInfo.PSODesc;

        // Pipeline state name is used by the engine to report issues
        // It is always a good idea to give objects descriptive names
        PSODesc.Name = "Simple triangle PSO";

        // This is a graphics pipeline
        PSODesc.PipelineType = PIPELINE_TYPE_GRAPHICS;

        // This tutorial will render to a single render target
        PSOCreateInfo.GraphicsPipeline.NumRenderTargets = 1;
        // Set render target format which is the format of the swap chain's color buffer
        PSOCreateInfo.GraphicsPipeline.RTVFormats[0] = m_pSwapChain->GetDesc().ColorBufferFormat;
        // This tutorial will not use depth buffer
        PSOCreateInfo.GraphicsPipeline.DSVFormat = m_pSwapChain->GetDesc().DepthBufferFormat;
        // Primitive topology defines what kind of primitives will be rendered by this pipeline state
        PSOCreateInfo.GraphicsPipeline.PrimitiveTopology = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        // No back face culling for this tutorial
        PSOCreateInfo.GraphicsPipeline.RasterizerDesc.CullMode = CULL_MODE_BACK;
        // Disable depth testing
        PSOCreateInfo.GraphicsPipeline.DepthStencilDesc.DepthEnable = True;

        ShaderCreateInfo ShaderCI;
        // Tell the system that the shader source code is in HLSL.
        // For OpenGL, the engine will convert this into GLSL behind the scene
        ShaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;
        // OpenGL backend requires emulated combined HLSL texture samplers (g_Texture + g_Texture_sampler combination)
        ShaderCI.Desc.UseCombinedTextureSamplers = true;
        // Create vertex shader
        RefCntAutoPtr<IShader> pVS;
        {
            ShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
            ShaderCI.EntryPoint = "main";
            ShaderCI.Desc.Name = "Triangle vertex shader";
            ShaderCI.Source = VSSource;
            m_pDevice->CreateShader(ShaderCI, &pVS);
        }

        // Create pixel shader
        RefCntAutoPtr<IShader> pFS;
        {
            ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
            ShaderCI.EntryPoint = "main";
            ShaderCI.Desc.Name = "Triangle pixel shader";
            ShaderCI.Source = PSSource;
            m_pDevice->CreateShader(ShaderCI, &pFS);
        }

        // Finally, create the pipeline state
        PSOCreateInfo.pVS = pVS;
        PSOCreateInfo.pPS = pFS;
        m_pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &m_pPSO);
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

        // Set pipeline state in the immediate context
        m_pImmediateContext->SetPipelineState(m_pPSO);
    }

    void DiligentRendererContext::EndFrame() {
        m_pSwapChain->Present(0);
    }

    void DiligentRendererContext::Reset(const uint32_t width, const uint32_t height) {
        if (m_pSwapChain) {
            m_pSwapChain->Resize(width, height);
        }
    }

    void DiligentRendererContext::StartSceneRender(FrameBufferHandle frameBuffer, const float4x4& view,
                                                   const float4x4& projection) {
    }

    ShaderHandle DiligentRendererContext::CreateShader(Buffer vertex, Buffer fragment, const std::string& name) {
        // TODO:
        return {};
    }

    bool DiligentRendererContext::RecreateShader(ShaderHandle handle, Buffer vertex, Buffer fragment) {
        // TODO:
        return {};
    }

    void DiligentRendererContext::Destroy(ShaderHandle shaderHandle) {
        // TODO:
    }

    void DiligentRendererContext::Destroy(VertexBufferHandle vertexBufferHandle) {
        // TODO:
    }

    UniformBufferHandle DiligentRendererContext::CreateUniformBuffer(const std::string& name,
                                                                     UniformBufferType uniformType, uint32_t count) {
        // TODO:
        return {};
    }

    void DiligentRendererContext::SetUniformData(UniformBufferHandle uniformBufferHandle, const void* data,
                                                 uint32_t count) {
        // TODO:
    }

    void DiligentRendererContext::Destroy(UniformBufferHandle uniformBufferHandle) {
        // TODO:
    }

    namespace TextureUtil {
        static TEXTURE_FORMAT GetTextureFormat(const ImageFormat imageFormat) {
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

        static RESOURCE_DIMENSION GetTextureType(const TextureType textureType) {
            switch (textureType) {
            case TextureType::Texture2D:
                return RESOURCE_DIM_TEX_2D;
            case TextureType::TextureCube:
                return RESOURCE_DIM_TEX_CUBE;
            }

            return RESOURCE_DIM_UNDEFINED;
        }

        static BIND_FLAGS GetBindFlags(const TextureSpecification& spec) {
            BIND_FLAGS bindFlags = BIND_NONE;
            if (spec.RenderTarget == TextureRenderTarget::ReadWrite) {
                bindFlags |= BIND_SHADER_RESOURCE;
                bindFlags |= BIND_RENDER_TARGET;
            }

            if (spec.Format == ImageFormat::DEPTH32F
                || spec.Format == ImageFormat::DEPTH24STENCIL8
                || spec.Format == ImageFormat::DEPTH32FSTENCIL8UINT
            ) {
                bindFlags |= BIND_DEPTH_STENCIL;
            }

            return bindFlags;
        }

        static void CreateTexture(const RefCntAutoPtr<IRenderDevice>& device, QTextureData& textureData) {
            const TextureSpecification& spec = textureData.Specification;

            TextureDesc textureDesc;
            textureDesc.Name = spec.Name.c_str();
            textureDesc.Format = GetTextureFormat(spec.Format);
            textureDesc.Width = spec.Width;
            textureDesc.Height = spec.Height;
            textureDesc.Type = GetTextureType(spec.Type);
            textureDesc.BindFlags = GetBindFlags(spec);

            device->CreateTexture(textureDesc, nullptr, &textureData.Texture);
        }

        void Resize(
            const RefCntAutoPtr<IRenderDevice>& device,
            QTextureData& data,
            const uint32_t width,
            const uint32_t height
        ) {
            data.Texture->Release();

            data.Specification.Width = width;
            data.Specification.Height = height;

            CreateTexture(device, data);
        }
    }

    TextureHandle DiligentRendererContext::CreateTexture(const TextureSpecification& spec) {
        const Handle<Texture> handle = m_TextureTable.Emplace();
        QTextureData* slot = m_TextureTable.Get(handle);
        slot->Specification = spec;
        TextureUtil::CreateTexture(m_pDevice, *slot);

        return handle;
    }

    TextureHandle DiligentRendererContext::CreateTexture(const TextureSpecification& spec, Buffer data) {
        return CreateTexture(spec);
    }

    TextureHandle DiligentRendererContext::CreateTexture(const TextureSpecification& spec,
                                                         const std::filesystem::path& path) {
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
        TextureUtil::Resize(m_pDevice, *m_TextureTable.Get(textureHandle), width, height);
    }

    const TextureSpecification* DiligentRendererContext::GetSpecification(const TextureHandle textureHandle) {
        const auto* data = m_TextureTable.Get(textureHandle);
        if (!data) {
            return nullptr;
        }

        return &data->Specification;
    }

    uint64_t DiligentRendererContext::TextureGetNativeHandle(const TextureHandle textureHandle) {
        const auto* data = m_TextureTable.Get(textureHandle);
        return reinterpret_cast<uint64_t>(data->Texture->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));
    }

    void DiligentRendererContext::Bind(TextureHandle textureHandle) {
        // TODO:
    }

    void DiligentRendererContext::Destroy(TextureHandle textureHandle) {
        // TODO:
    }

    RenderPassHandle DiligentRendererContext::CreateRenderPass() {
        const Handle<RenderPass> handle = m_RenderPassTable.Emplace();
        IRenderPass** slot = m_RenderPassTable.Get(handle);

        RenderPassAttachmentDesc Attachments[2];

        // Color
        Attachments[0].Format = TEX_FORMAT_RGBA8_UNORM;
        Attachments[0].SampleCount = 1;
        Attachments[0].LoadOp = ATTACHMENT_LOAD_OP_CLEAR;
        Attachments[0].StoreOp = ATTACHMENT_STORE_OP_STORE;
        Attachments[0].InitialState = RESOURCE_STATE_UNDEFINED;
        Attachments[0].FinalState = RESOURCE_STATE_SHADER_RESOURCE;

        // Depth
        Attachments[1].Format = TEX_FORMAT_D32_FLOAT;
        Attachments[1].SampleCount = 1;
        Attachments[1].LoadOp = ATTACHMENT_LOAD_OP_CLEAR;
        Attachments[1].StoreOp = ATTACHMENT_STORE_OP_DISCARD;
        Attachments[1].InitialState = RESOURCE_STATE_UNDEFINED;
        Attachments[1].FinalState = RESOURCE_STATE_DEPTH_WRITE;

        AttachmentReference ColorRef = {0, RESOURCE_STATE_RENDER_TARGET};
        AttachmentReference DepthRef = {1, RESOURCE_STATE_DEPTH_WRITE};

        SubpassDesc Subpass;
        Subpass.RenderTargetAttachmentCount = 1;
        Subpass.pRenderTargetAttachments = &ColorRef;
        Subpass.pDepthStencilAttachment = &DepthRef;

        RenderPassDesc RPDesc;
        RPDesc.Name = "View Render Pass";
        RPDesc.AttachmentCount = 2;
        RPDesc.pAttachments = Attachments;
        RPDesc.SubpassCount = 1;
        RPDesc.pSubpasses = &Subpass;

        m_pDevice->CreateRenderPass(RPDesc, slot);

        return handle;
    }

    void DiligentRendererContext::Destroy(const RenderPassHandle renderPassHandle) {
        (*m_RenderPassTable.Get(renderPassHandle))->Release();
        m_RenderPassTable.Erase(renderPassHandle);
    }

    namespace Utils {
        static void CreateFrameBuffer(IFramebuffer*& frameBuffer, IRenderDevice* device,
                                      const Span<ITexture*> attachments, IRenderPass* renderPass) {
            SmallVec<ITextureView*, 2> viewAttachments;
            viewAttachments.reserve(attachments.size());
            for (uint64_t i = 0; i < attachments.size() - 1; i++) {
                viewAttachments.push_back(attachments[i]->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET));
            }
            viewAttachments.push_back(attachments[attachments.size() - 1]->GetDefaultView(TEXTURE_VIEW_DEPTH_STENCIL));

            FramebufferDesc desc;
            desc.Name = "FrameBuffer";
            desc.ppAttachments = viewAttachments.data();
            desc.AttachmentCount = viewAttachments.size();
            if (renderPass) {
                desc.pRenderPass = renderPass;
            }
            desc.Width = attachments[0]->GetDesc().GetWidth();
            desc.Height = attachments[0]->GetDesc().GetHeight();
            desc.NumArraySlices = 1;

            device->CreateFramebuffer(desc, &frameBuffer);
        }
    }

    FrameBufferHandle DiligentRendererContext::CreateFrameBuffer(const FrameBufferSpec& frameBufferSpec) {
        const Handle<FrameBuffer> handle = m_FrameBufferTable.Emplace();
        QFrameBufferData* slot = m_FrameBufferTable.Get(handle);

        // Own data
        slot->Name = frameBufferSpec.Name;
        slot->Attachments = SmallVec<TextureHandle, 2>(frameBufferSpec.Attachments);

        FrameBufferSpec& spec = slot->Specification;
        spec = { slot->Name, slot->Attachments, frameBufferSpec.RenderPassHandle, frameBufferSpec.Size };

        SmallVec<ITexture*, 2> textureAttachments;
        textureAttachments.reserve(spec.Attachments.size());
        for (uint64_t i = 0; i < spec.Attachments.size(); i++) {
            textureAttachments.push_back(m_TextureTable.Get(spec.Attachments[i])->Texture);
        }

        auto& textureDesc = textureAttachments[0]->GetDesc();
        QS_CORE_ASSERT(
            spec.Size.Width == textureDesc.Width && spec.Size.Height == textureDesc.Height,
            "Frame buffer size does not match texture size!"
        );

        Utils::CreateFrameBuffer(
            slot->FrameBuffer,
            m_pDevice,
            Span(textureAttachments),
            *m_RenderPassTable.Get(spec.RenderPassHandle)
        );

        return handle;
    }

    uint32_t DiligentRendererContext::FrameBufferGetWidth(const FrameBufferHandle frameBufferHandle) {
        const QFrameBufferData* slot = m_FrameBufferTable.Get(frameBufferHandle);
        return slot->Specification.Size.Width;
    }

    uint32_t DiligentRendererContext::FrameBufferGetHeight(const FrameBufferHandle frameBufferHandle) {
        const QFrameBufferData* slot = m_FrameBufferTable.Get(frameBufferHandle);
        return slot->Specification.Size.Height;
    }

    Extent2D DiligentRendererContext::FrameBufferGetSize(const FrameBufferHandle frameBufferHandle) {
        const QFrameBufferData* slot = m_FrameBufferTable.Get(frameBufferHandle);
        return slot->Specification.Size;
    }

    void DiligentRendererContext::FrameBufferResize(
        const FrameBufferHandle frameBufferHandle,
        const uint32_t width,
        const uint32_t height
    ) {
        QFrameBufferData* data = m_FrameBufferTable.Get(frameBufferHandle);
        data->Specification.Size = { width, height };

        SmallVec<ITexture*, 2> textureAttachments;
        for (const TextureHandle attachment : data->Attachments) {
            QTextureData* texture = m_TextureTable.Get(attachment);
            TextureUtil::Resize(m_pDevice, *texture, width, height);
            textureAttachments.push_back(texture->Texture);
        }

        data->FrameBuffer->Release();
        data->FrameBuffer = nullptr;

        IRenderPass* renderPass = nullptr;
        if (data->Specification.RenderPassHandle.IsValid()) {
            renderPass = *m_RenderPassTable.Get(data->Specification.RenderPassHandle);
        }

        Utils::CreateFrameBuffer(data->FrameBuffer, m_pDevice, Span(textureAttachments), renderPass);
    }

    void DiligentRendererContext::Destroy(const FrameBufferHandle frameBufferHandle) {
        m_FrameBufferTable.Erase(frameBufferHandle);
    }

    void DiligentRendererContext::SubmitMesh(
        const MeshRenderer& mesh,
        const WorldTransform& transform
    ) {
        return;
        Mesh& meshAsset = mesh.MeshData.Get();
        BindVertexBuffer(meshAsset.GetVertexBuffer(), 0);
        BindIndexBuffer(meshAsset.GetIndexBuffer());

        // TODO: Set shader resources

        DrawIndexedAttribs drawAttrs; // This is an indexed draw call
        drawAttrs.IndexType = VT_UINT16; // Index type
        drawAttrs.NumIndices = meshAsset.GetIndices().size();

        // Verify the state of vertex and index buffers
        drawAttrs.Flags = DRAW_FLAG_VERIFY_ALL;
        m_pImmediateContext->DrawIndexed(drawAttrs);
    }

    void DiligentRendererContext::Submit(ShaderHandle shaderHandle, uint32_t view) {
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
        IBuffer** slot = m_VertexBufferTable.Get(handle);

        m_pDevice->CreateBuffer(vertBuffDesc, &vbData, slot);

        return handle;
    }

    void DiligentRendererContext::BindVertexBuffer(VertexBufferHandle vertexBufferHandle, uint32_t stream) {
        IBuffer** slot = m_VertexBufferTable.Get(vertexBufferHandle);
        if (!slot) {
            QS_CORE_ERROR_TAG(
                "RendererContext",
                "No vertex buffer found with handle ({},{})",
                vertexBufferHandle.GetIndex(), vertexBufferHandle.GetGeneration()
            );

            return;
        }

        const Uint64 offset = 0;
        m_pImmediateContext->SetVertexBuffers(stream, 1, slot, &offset, RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
                                              SET_VERTEX_BUFFERS_FLAG_RESET);
    }

    IndexBufferHandle DiligentRendererContext::CreateIndexBuffer(const Span<uint16_t> indices) {
        BufferDesc indBuffDesc;
        indBuffDesc.Name = "IndexBuffer";
        indBuffDesc.Usage = USAGE_IMMUTABLE;
        indBuffDesc.BindFlags = BIND_INDEX_BUFFER;
        indBuffDesc.Size = indices.size();

        BufferData ibData;
        ibData.pData = indices.data();
        ibData.DataSize = indices.size();

        const Handle<IndexBuffer> handle = m_IndexBufferTable.Emplace();
        IBuffer** slot = m_IndexBufferTable.Get(handle);

        m_pDevice->CreateBuffer(indBuffDesc, &ibData, slot);
        return handle;
    }

    void DiligentRendererContext::BindIndexBuffer(const IndexBufferHandle indexBufferHandle) {
        IBuffer** slot = m_IndexBufferTable.Get(indexBufferHandle);
        if (!slot) [[unlikely]] {
            QS_CORE_ERROR_TAG(
                "RendererContext",
                "No vertex buffer found with handle ({},{})",
                indexBufferHandle.GetIndex(), indexBufferHandle.GetGeneration()
            );

            return;
        }

        m_pImmediateContext->SetIndexBuffer(*slot, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    }

    void DiligentRendererContext::Destroy(const IndexBufferHandle indexBufferHandle) {
        IBuffer** slot = m_IndexBufferTable.Get(indexBufferHandle);
        if (!slot) [[unlikely]] {
            QS_CORE_ERROR_TAG(
                "RendererContext",
                "No index buffer found with handle ({},{})",
                indexBufferHandle.GetIndex(), indexBufferHandle.GetGeneration()
            );

            return;
        }

        (*slot)->Release();
        m_IndexBufferTable.Erase(indexBufferHandle);
    }

    void DiligentRendererContext::Shutdown() {
        m_pImmediateContext->Flush();
    }
}
