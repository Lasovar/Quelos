#pragma once

#include "Shader.h"
#include "Quelos/AssetManager/AssetRef.h"

namespace Quelos {
    class QS_API Material {
    public:
        explicit Material();

        [[nodiscard]] const AssetRef<Shader>& GetShader() const { return m_Shader; }
        [[nodiscard]] ShaderHandle GetShaderHandle() const;

    private:
        AssetRef<Shader> m_Shader;
    };
}
