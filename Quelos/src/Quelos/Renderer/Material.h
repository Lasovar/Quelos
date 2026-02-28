#pragma once

namespace Quelos {
    class Shader;

    class Material {
    public:
        explicit Material(const Ref<Shader>& shader);

        [[nodiscard]] Ref<Shader> GetShader() const;

    private:
        Ref<Shader> m_Shader;
    };
}
