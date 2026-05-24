#pragma once

#include "GraphicsShader.h"
#include "Quelos/AssetManager/AssetRef.h"

namespace Quelos {
    class QS_API Material : public Asset {
    public:
        explicit Material();

        [[nodiscard]] const AssetRef<GraphicsShader>& GetShader() const { return m_Shader; }

    private:
        AssetRef<GraphicsShader> m_Shader;
    };
}
