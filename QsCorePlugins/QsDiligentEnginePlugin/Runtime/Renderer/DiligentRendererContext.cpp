#include "DiligentRendererContext.h"

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

        switch (api) {
#if VULKAN_SUPPORTED
        case RendererAPI::Vulkan: {
            auto* pFactoryVk = GetEngineFactoryVk();
            const EngineVkCreateInfo engineCi;

            pFactoryVk->CreateDeviceAndContextsVk(engineCi, &m_pDevice, &m_pImmediateContext);

            SwapChainDesc SCDesc;
            SCDesc.Width = window->GetWidth();
            SCDesc.Height = window->GetHeight();

            LinuxNativeWindow linuxWindow;

            linuxWindow.pDisplay = window->GetNativeDisplay();
            if (window->IsWayland()) {
                linuxWindow.pWaylandSurface = window->GetNativeWindow();
            }
            else {
                linuxWindow.pXCBConnection = window->GetNativeWindow();
            }

            pFactoryVk->CreateSwapChainVk(m_pDevice, m_pImmediateContext, SCDesc, linuxWindow, &m_pSwapChain);
            break;
        }
#endif
#if GL_SUPPORTED
        case RendererAPI::OpenGL: {
            SwapChainDesc SCDesc;
            // Declare function pointer
            auto* pFactoryOpenGL = GetEngineFactoryOpenGL();

            EngineGLCreateInfo engineCi;

            engineCi.Window.pDisplay = window->GetNativeDisplay();
            if (window->IsWayland()) {
                engineCi.Window.pWaylandSurface = window->GetNativeWindow();
            }
            else {
                engineCi.Window.pXCBConnection = window->GetNativeWindow();
            }

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
        PSOCreateInfo.GraphicsPipeline.DSVFormat = TEX_FORMAT_D32_FLOAT;
        // Primitive topology defines what kind of primitives will be rendered by this pipeline state
        PSOCreateInfo.GraphicsPipeline.PrimitiveTopology = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        // No back face culling for this tutorial
        PSOCreateInfo.GraphicsPipeline.RasterizerDesc.CullMode = CULL_MODE_NONE;
        // Disable depth testing
        PSOCreateInfo.GraphicsPipeline.DepthStencilDesc.DepthEnable = False;

        ShaderCreateInfo ShaderCI;
        // Tell the system that the shader source code is in HLSL.
        // For OpenGL, the engine will convert this into GLSL behind the scene
        ShaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;
        // OpenGL backend requires emulated combined HLSL texture samplers (g_Texture + g_Texture_sampler combination)
        ShaderCI.Desc.UseCombinedTextureSamplers = true;
        // Create vertex shader
        RefCntAutoPtr<IShader> pVS; {
            ShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
            ShaderCI.EntryPoint = "main";
            ShaderCI.Desc.Name = "Triangle vertex shader";
            ShaderCI.Source = VSSource;
            m_pDevice->CreateShader(ShaderCI, &pVS);
        }

        // Create pixel shader
        RefCntAutoPtr<IShader> pFS; {
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
        m_pSwapChain->Present();
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
    }

    bool DiligentRendererContext::RecreateShader(ShaderHandle handle, Buffer vertex, Buffer fragment) {
    }

    void DiligentRendererContext::Destroy(ShaderHandle shaderHandle) {
    }

    void DiligentRendererContext::Destroy(VertexBufferHandle vertexBufferHandle) {
    }

    UniformBufferHandle DiligentRendererContext::CreateUniformBuffer(const std::string& name,
        UniformBufferType uniformType, uint32_t count) {
    }

    void DiligentRendererContext::SetUniformData(UniformBufferHandle uniformBufferHandle, const void* data,
        uint32_t count) {
    }

    void DiligentRendererContext::Destroy(UniformBufferHandle uniformBufferHandle) {
    }

    TextureHandle DiligentRendererContext::CreateTexture(const TextureSpecification& spec) {
    }

    TextureHandle DiligentRendererContext::CreateTexture(const TextureSpecification& spec, Buffer data) {
    }

    TextureHandle DiligentRendererContext::CreateTexture(const TextureSpecification& spec,
        const std::filesystem::path& path) {
    }

    bool DiligentRendererContext::TextureIsVFlipped() {
    }

    void DiligentRendererContext::TextureResize(TextureHandle textureHandle, uint32_t width, uint32_t height) {
    }

    const TextureSpecification* DiligentRendererContext::GetSpecification(TextureHandle textureHandle) {
    }

    uint16_t DiligentRendererContext::TextureGetNativeHandle(TextureHandle textureHandle) {
    }

    void DiligentRendererContext::Bind(TextureHandle textureHandle) {
    }

    void DiligentRendererContext::Destroy(TextureHandle textureHandle) {
    }

    FrameBufferHandle DiligentRendererContext::CreateFrameBuffer(uint32_t viewID, Span<TextureHandle> attachments) {
    }

    uint32_t DiligentRendererContext::FrameBufferGetWidth(FrameBufferHandle frameBufferHandle) {
    }

    uint32_t DiligentRendererContext::FrameBufferGetHeight(FrameBufferHandle frameBufferHandle) {
    }

    uint2 DiligentRendererContext::FrameBufferGetSize(FrameBufferHandle frameBufferHandle) {
    }

    void DiligentRendererContext::FrameBufferSetViewID(FrameBufferHandle frameBufferHandle, uint32_t viewId) {
    }

    uint32_t DiligentRendererContext::FrameBufferGetViewID(FrameBufferHandle frameBufferHandle) {
    }

    void DiligentRendererContext::FrameBufferResize(FrameBufferHandle frameBufferHandle, uint32_t width,
        uint32_t height) {
    }

    void DiligentRendererContext::Bind(FrameBufferHandle frameBufferHandle) {
    }

    void DiligentRendererContext::Destroy(FrameBufferHandle frameBufferHandle) {
    }

    void DiligentRendererContext::SubmitMesh(
        uint32_t viewID,
        const MeshRenderer& mesh,
        const WorldTransform& transform
    ) {
        Mesh& meshAsset = mesh.MeshData.Get();
        BindVertexBuffer(meshAsset.GetVertexBuffer(), 0);
        BindIndexBuffer(meshAsset.GetIndexBuffer());

        // TODO: Set shader resources

        DrawIndexedAttribs drawAttrs;     // This is an indexed draw call
        drawAttrs.IndexType = VT_UINT16; // Index type
        drawAttrs.NumIndices = meshAsset.GetIndices().size();

        // Verify the state of vertex and index buffers
        drawAttrs.Flags = DRAW_FLAG_VERIFY_ALL;
        m_pImmediateContext->DrawIndexed(drawAttrs);
    }

    void DiligentRendererContext::Submit(ShaderHandle shaderHandle, uint32_t view) {
        // TODO:
    }

    VertexBufferHandle DiligentRendererContext::CreateVertexBuffer(const BufferView vertices, VertexLayout bufferLayout) {
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
        m_pImmediateContext->SetVertexBuffers(stream, 1, slot, &offset, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
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
