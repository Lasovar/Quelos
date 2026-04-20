#include "Shader.h"
#include "Renderer.h"

namespace Quelos {

    void ShaderHandle::Submit(const uint32_t viewId) const {
        Renderer::Submit(*this, viewId);
    }

    Shader::Shader(Buffer vertex, Buffer fragment, std::string name) : m_Name(std::move(name)) {
        m_ShaderHandle = Renderer::CreateShader(std::move(vertex), std::move(fragment), name);
    }

    void Shader::Recreate(Buffer vertex, Buffer fragment) const {
        Renderer::RecreateShader(m_ShaderHandle, std::move(vertex), std::move(fragment));
    }

    Shader::~Shader() {
        Renderer::Destroy(m_ShaderHandle);
    }
}
