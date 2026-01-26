#include "qspch.h"

#include "Material.h"
#include "Quelos/Renderer/Shader.h"

namespace Quelos {
    Material::Material(const Ref<Shader>& shader) {
        m_Shader = shader;
    }

    Ref<Shader> Material::GetShader() const { return m_Shader; }
}
