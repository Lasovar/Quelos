#include "qspch.h"

#include "Material.h"

#include "Renderer.h"
#include "Quelos/Renderer/Shader.h"

namespace Quelos {
    Material::Material() {
        m_Shader = AssetManager::GetAsset<Shader>(AssetHandle("af5fda92-37f9-42e3-a189-3a5388090a14"));
    }

    ShaderHandle Material::GetShaderHandle() const { return m_Shader->GetHandle(); }
}
