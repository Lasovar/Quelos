#include "Shader.h"

#include "PipelineState.h"
#include "Renderer.h"
#include "RenderResource.h"

namespace Quelos {
    void GraphicsShaderHandle::Submit(const uint32_t viewId) const {
        Renderer::Submit(*this, viewId);
    }

    GraphicsShader::GraphicsShader(Buffer vertex, Buffer fragment, std::string name) : m_Name(std::move(name)),
        m_VertexBlob(std::move(vertex)), m_FragmentBlob(std::move(fragment)) {}

    void GraphicsShader::CreatePSO(RenderPassHandle renderPassHandle, const GPUBufferHandle globalBufferHandle) {
        ShaderCreateInfo vertexCreateInfo;
        vertexCreateInfo.Specification.Name = m_Name;
        vertexCreateInfo.Specification.Type = ShaderType::Vertex;
        vertexCreateInfo.Specification.EntryPoint = "vertexMain";
        vertexCreateInfo.ByteCode = m_VertexBlob;

        ScopedRenderResource vertexShader = Renderer::CreateShader(vertexCreateInfo);

        ShaderCreateInfo fragmentCreateInfo;
        fragmentCreateInfo.Specification.Name = m_Name;
        fragmentCreateInfo.Specification.Type = ShaderType::Fragment;
        fragmentCreateInfo.Specification.EntryPoint = "fragmentMain";
        fragmentCreateInfo.ByteCode = m_FragmentBlob;

        ScopedRenderResource fragmentShader = Renderer::CreateShader(fragmentCreateInfo);

        GraphicsPipelineStateCreateInfo pipelineStateCreateInfo;
        pipelineStateCreateInfo.Name = m_Name;
        pipelineStateCreateInfo.GraphicsPipeline.RenderPass = renderPassHandle;
        pipelineStateCreateInfo.GraphicsPipeline.RasterizerSpec.CullMode = CullMode::Back;
        pipelineStateCreateInfo.GraphicsPipeline.RasterizerSpec.FrontCounterClockwise = true;
        pipelineStateCreateInfo.GraphicsPipeline.DepthStencilSpec.DepthEnable = true;

        LayoutElementBuilder<4> layoutBuilder {
            LayoutElement{0, 0, ShaderDataType::Float3},
            LayoutElement{1, 0, ShaderDataType::Float3},
            LayoutElement{2, 0, ShaderDataType::Float3},
            LayoutElement{3, 0, ShaderDataType::Float2}
        };

        pipelineStateCreateInfo.GraphicsPipeline.InputLayout.LayoutElements = layoutBuilder;

        pipelineStateCreateInfo.VertexShader = vertexShader.Handle;
        pipelineStateCreateInfo.FragmentShader = fragmentShader.Handle;

        ShaderResourceVariableSpec vars[] = {
            {"global", ShaderType::VertexAndFragment, ShaderResourceVariableType::Static},
            //{ "materials",ShaderType::Fragment,           ShaderResourceVariableType::Mutable },
        };

        pipelineStateCreateInfo.Spec.ResourceLayout.Variables = vars;

        m_PipelineStateHandle = Renderer::CreatePipelineState(pipelineStateCreateInfo);

        Renderer::BindStaticVariableByName(m_PipelineStateHandle, ShaderType::Vertex, "global", globalBufferHandle);

        m_VertexBlob.release();
        m_FragmentBlob.release();
    }

    void GraphicsShader::Recreate(Buffer vertex, Buffer fragment) {
        m_VertexBlob = std::move(vertex);
        m_FragmentBlob = std::move(fragment);

        Renderer::Destroy(m_PipelineStateHandle);

        m_PipelineStateHandle = {};
    }

    GraphicsShader::~GraphicsShader() {
        if (m_PipelineStateHandle.IsValid()) {
            Renderer::Destroy(m_PipelineStateHandle);
        }
    }
}
