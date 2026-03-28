#include "qspch.h"

#include "Material.h"

#include "Renderer.h"
#include "Quelos/Renderer/Shader.h"

namespace Quelos {
    Material::Material(const ShaderHandle& shader) {
        m_Shader = shader;
    }

    Material::Material(const std::string& filePathVertex, const std::string& filePathFragment) {
        m_Shader = Renderer::CreateShader(filePathVertex, filePathFragment);
    }

    ShaderHandle Material::GetShader() const { return m_Shader; }
}
