//
// Created by lasovar on 6/28/26.
//

#include "ComputeShader.h"

#include "Renderer.h"

namespace Quelos {
    ComputeShader::ComputeShader(const ComputeShaderCreateInfo& createInfo) {
        m_Name = createInfo.Name;

        ShaderCreateInfo shaderCreateInfo;
        shaderCreateInfo.Specification.Name = m_Name;
        shaderCreateInfo.Specification.Type = ShaderType::Compute;
        shaderCreateInfo.Specification.EntryPoint = createInfo.EntryPoint;
        shaderCreateInfo.ByteCode = createInfo.Code;

        m_Shader = Renderer::CreateShader(shaderCreateInfo);
        m_ThreadGroupSize = createInfo.ThreadGroupSize;
    }

    ComputeShader::~ComputeShader() {
        Renderer::Destroy(m_Shader);
    }
}
