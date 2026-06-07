//
// Created by lasovar on 6/7/26.
//

#include "PipelineState.h"
#include "Renderer.h"
#include "GraphicsShader.h"

namespace Quelos {
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
