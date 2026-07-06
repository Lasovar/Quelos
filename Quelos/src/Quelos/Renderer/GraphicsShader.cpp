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
        for (const auto& [passName, shaders] : createInfo.Passes) {
            GraphicsShaderPass pass;
            pass.Name = passName;
            pass.Variables = createInfo.Variables;

            for (const auto & shader : shaders) {
                if (shader.Type != ShaderType::Vertex && shader.Type != ShaderType::Fragment) {
                    continue;
                }

                GraphicsShaderPipelineData* pipelineData = nullptr;

                auto it = std::ranges::find(pass.Pipelines, shader.Order, &GraphicsShaderPipelineData::Order);
                if (it == pass.Pipelines.end()) {
                    pipelineData = &pass.Pipelines.emplace_back();
                    pipelineData->Order = shader.Order;
                } else {
                    pipelineData = &*it;
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

                for (const auto& pipelineOption : shader.PipelineOptions) {
                    pipelineData->PipelineOptions[pipelineOption.first] = pipelineOption.second;
                }

                if (shader.Type == ShaderType::Vertex) {
                    pipelineData->VertexShader = Renderer::CreateShader(shaderCreateInfo);
                } else {
                    pipelineData->FragmentShader = Renderer::CreateShader(shaderCreateInfo);
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
            if (Renderer::IsAlive(pipelineState)) {
                Renderer::Destroy(pipelineState);
            }
        }

        for (const GraphicsShaderPass& pass : m_Passes) {
            for (const auto & pipeline : pass.Pipelines) {
                Renderer::Destroy(pipeline.VertexShader);
                Renderer::Destroy(pipeline.FragmentShader);
            }
        }
    }
}
