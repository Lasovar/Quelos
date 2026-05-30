#include "Shader.h"

#include "PipelineState.h"
#include "Renderer.h"
#include "RenderResource.h"

namespace Quelos {
    void GraphicsShaderHandle::Submit(const uint32_t viewId) const {
        Renderer::Submit(*this, viewId);
    }

    GraphicsShader::GraphicsShader(
        Buffer vertex,
        Buffer fragment,
        std::string name,
        const Vec<MaterialPropertySpec>& materialProperties,
        const uint64_t materialSize
    ) : m_Name(std::move(name)), m_MaterialProperties(materialProperties), m_MaterialSize(materialSize) {
        ShaderCreateInfo vertexCreateInfo;
        vertexCreateInfo.Specification.Name = m_Name;
        vertexCreateInfo.Specification.Type = ShaderType::Vertex;
        vertexCreateInfo.Specification.EntryPoint = "vertexMain";
        vertexCreateInfo.ByteCode = vertex;

        m_VertexShader = Renderer::CreateShader(vertexCreateInfo);
        vertex.release();

        ShaderCreateInfo fragmentCreateInfo;
        fragmentCreateInfo.Specification.Name = m_Name;
        fragmentCreateInfo.Specification.Type = ShaderType::Fragment;
        fragmentCreateInfo.Specification.EntryPoint = "fragmentMain";
        fragmentCreateInfo.ByteCode = fragment;

        m_FragmentShader = Renderer::CreateShader(fragmentCreateInfo);
        fragment.release();
    }

    void GraphicsShader::Recreate(Buffer vertex, Buffer fragment) {
        // TODO:
        ShaderCreateInfo vertexCreateInfo;
        vertexCreateInfo.Specification.Name = m_Name;
        vertexCreateInfo.Specification.Type = ShaderType::Vertex;
        vertexCreateInfo.Specification.EntryPoint = "vertexMain";
        vertexCreateInfo.ByteCode = vertex;

        m_VertexShader = Renderer::CreateShader(vertexCreateInfo);
        vertex.release();

        ShaderCreateInfo fragmentCreateInfo;
        fragmentCreateInfo.Specification.Name = m_Name;
        fragmentCreateInfo.Specification.Type = ShaderType::Fragment;
        fragmentCreateInfo.Specification.EntryPoint = "fragmentMain";
        fragmentCreateInfo.ByteCode = fragment;

        m_FragmentShader = Renderer::CreateShader(fragmentCreateInfo);
        fragment.release();
    }

    GraphicsShader::~GraphicsShader() {
        if (!Renderer::IsInitialized()) [[unlikely]] {
            return;
        }

        for (const PipelineStateHandle& pipelineState : m_PipelineStates) {
            Renderer::Destroy(pipelineState);
        }

        Renderer::Destroy(m_FragmentShader);
        Renderer::Destroy(m_VertexShader);
    }
}
