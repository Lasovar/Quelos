#pragma once

namespace Quelos {
    class Shader;

    class Material : public RefCounted {
    public:
        explicit Material(const Ref<Shader>& shader);

        Ref<Shader> GetShader() const;

    private:
        Ref<Shader> m_Shader;
    };
}
