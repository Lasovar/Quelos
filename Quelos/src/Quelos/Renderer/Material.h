#pragma once

#include "Shader.h"

namespace Quelos {
    class QS_API Material {
    public:
        explicit Material();

        [[nodiscard]] const Ref<Shader>& GetShader() const { return m_Shader; }
        [[nodiscard]] ShaderHandle GetShaderHandle() const;

    private:
        Ref<Shader> m_Shader;
    };
}
