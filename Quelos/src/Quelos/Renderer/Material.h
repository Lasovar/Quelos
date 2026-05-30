#pragma once

#include "GraphicsShader.h"
#include "Quelos/AssetManager/AssetRef.h"

namespace Quelos {
    class QS_API Material : public Asset {
    public:
        explicit Material();
        void SetShader(const AssetID shaderId) {
            m_MaterialData.release();
            m_MaterialProperties.clear();
            m_Shader = AssetRef<GraphicsShader>(shaderId);

            if (const GraphicsShader* graphicsShader = m_Shader.TryGet()) {
                m_MaterialData = Buffer::Allocate(graphicsShader->GetMaterialSize());
                m_MaterialProperties = graphicsShader->GetMaterialProperties();
            }
        }

        [[nodiscard]] const AssetRef<GraphicsShader>& GetShader() const { return m_Shader; }
        [[nodiscard]] BufferView GetBufferView() const { return m_MaterialData.view(); }
        [[nodiscard]] MutBufferView GetMutBufferView() { return m_MaterialData.mut_view(); }
        [[nodiscard]] const Vec<MaterialPropertySpec>& GetMaterialProperties() const { return m_MaterialProperties; }

        void SetProperty(const uint64_t offset, const uint64_t size, const void* value) const {
            std::memcpy(
                m_MaterialData.data() + offset,
                value,
                size
            );
        }

        template <typename T>
        T GetProperty(const uint64_t offset, const uint64_t size) const {
            QS_ASSERT(size <= sizeof(T));

            T result{};

            std::memcpy(
                &result,
                m_MaterialData.data() + offset,
                size
            );

            return result;
        }

    private:
        AssetRef<GraphicsShader> m_Shader;
        Vec<MaterialPropertySpec> m_MaterialProperties;
        Buffer m_MaterialData;

    public:
        static const AssetType& GetStaticType() {
            static AssetType assetType = Quelos::GetAssetType<Material>();
            return assetType;
        }

        [[nodiscard]] const AssetType& GetAssetType() const override { return GetStaticType(); }
    };
}
