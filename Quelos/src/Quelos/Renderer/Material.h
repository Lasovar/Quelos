#pragma once

#include "Shader.h"

namespace Quelos {
    class Material {
    public:
        explicit Material(const ShaderHandle& shader);
        Material(const std::string& filePathVertex, const std::string& filePathFragment);

        [[nodiscard]] ShaderHandle GetShader() const;

    private:
        ShaderHandle m_Shader;
    };
}
