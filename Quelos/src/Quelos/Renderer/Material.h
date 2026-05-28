#pragma once

#include "GraphicsShader.h"
#include "Quelos/AssetManager/AssetRef.h"

namespace Quelos {
    class QS_API Material : public Asset {
    public:
        explicit Material();
        void SetShader(const AssetRef<GraphicsShader>& shader) {
            m_MaterialData.release();
            m_MaterialProperties.clear();
            m_Shader = shader;

            if (const GraphicsShader* graphicsShader = m_Shader.TryGet()) {
                m_MaterialData = Buffer::Allocate(graphicsShader->GetMaterialSize());
                m_MaterialProperties = graphicsShader->GetMaterialProperties();
            }
        }

        [[nodiscard]] const AssetRef<GraphicsShader>& GetShader() const { return m_Shader; }
        BufferView GetBufferView() const { return m_MaterialData.view(); }

    private:
        AssetRef<GraphicsShader> m_Shader;
        Vec<MaterialProperty> m_MaterialProperties;
        Buffer m_MaterialData;
    };
}
