//
// Created by lasovar on 6/7/26.
//

#include "PipelineState.h"
#include "Renderer.h"
#include "GraphicsShader.h"

#include "magic_enum/magic_enum.hpp"

namespace Quelos {
    GraphicsShader::GraphicsShader(const GraphicsShaderCreateInfo& createInfo)
        : m_Name(createInfo.Name),
          m_MaterialProperties(createInfo.MaterialProperties.begin(), createInfo.MaterialProperties.end()),
          m_MaterialSize(createInfo.MaterialSize)
    {
        for (const auto & [passName, shaders] : createInfo.Passes) {
            GraphicsShaderPass pass;
            pass.Name = passName;

            for (const ShaderData& shader : shaders) {
                if (shader.Type != ShaderType::Vertex && shader.Type != ShaderType::Fragment) {
                    continue;
                }

                const std::string shaderName = FormatTemp(
                    "{}_{}_{}",
                    m_Name,
                    passName,
                    shader.EntryPoint
                );

                ShaderCreateInfo shaderCreateInfo;
                shaderCreateInfo.Specification.Name = shaderName;
                shaderCreateInfo.Specification.Type = shader.Type;
                shaderCreateInfo.Specification.EntryPoint = shader.EntryPoint;
                shaderCreateInfo.ByteCode = shader.Code;

                if (shader.Type == ShaderType::Vertex) {
                    pass.VertexShader = Renderer::CreateShader(shaderCreateInfo);
                } else {
                    pass.FragmentShader = Renderer::CreateShader(shaderCreateInfo);
                }
            }

            m_Passes.push_back(pass);
        }
    }

    GraphicsShader::~GraphicsShader() {
        if (!Renderer::IsInitialized()) [[unlikely]] {
            return;
        }

        for (const PipelineStateHandle& pipelineState : m_PipelineStates) {
            Renderer::Destroy(pipelineState);
        }

        for (const GraphicsShaderPass& pass : m_Passes) {
            Renderer::Destroy(pass.VertexShader);
            Renderer::Destroy(pass.FragmentShader);
        }
    }
}
