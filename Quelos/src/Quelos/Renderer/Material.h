#pragma once

#include "GraphicsShader.h"
#include "Quelos/AssetManager/AssetRef.h"

namespace Quelos {
    using MaterialPropertyValue = std::variant<
        float,
        float2,
        float3,
        float4,
        Color,
        AssetRef<Texture2D>
    >;

    class QS_API Material : public Asset {
    public:
        explicit Material();
        void SetShader(const AssetID shaderId) {
            m_MaterialProperties.clear();
            m_Values.clear();
            m_Shader = AssetRef<GraphicsShader>(shaderId);

            if (const GraphicsShader* graphicsShader = m_Shader.TryGet()) {
                m_MaterialProperties = graphicsShader->GetMaterialProperties();
                m_Values.resize(m_MaterialProperties.size());

                for (size_t i = 0; i < m_MaterialProperties.size(); i++) {
                    EnsureType(m_MaterialProperties[i].Type, m_Values[i]);
                }
            }
        }

        [[nodiscard]] const AssetRef<GraphicsShader>& GetShader() const { return m_Shader; }
        [[nodiscard]] const Vec<MaterialPropertySpec>& GetMaterialProperties() const { return m_MaterialProperties; }
        [[nodiscard]] const Vec<MaterialPropertyValue>& GetMaterialPropertyValues() const { return m_Values; }

        template<typename T>
        void SetProperty(const uint64_t offset, const T& value) {
            const size_t index = GetPropertyIndexByOffset(offset);
            if (index == m_MaterialProperties.size()) {
                return;
            }

            m_Values[index] = value;
        }

        template <typename T>
        const T* GetProperty(const uint64_t offset) const {
            const size_t index = GetPropertyIndexByOffset(offset);
            if (index == m_MaterialProperties.size()) {
                return nullptr;
            }

            return &std::get<T>(m_Values[index]);
        }
    private:
        [[nodiscard]] size_t GetPropertyIndexByOffset(const uint64_t offset) const {
            const auto it = std::ranges::lower_bound(
                m_MaterialProperties,
                offset,
                {},
                &MaterialPropertySpec::Offset
            );

            if (it == m_MaterialProperties.end() || it->Offset != offset) {
                return m_MaterialProperties.size();
            }

            return it - m_MaterialProperties.begin();
        }

    public:

        static void EnsureType(const MaterialPropertyType propertyType, MaterialPropertyValue& propertyValue) {
            switch (propertyType) {
            case MaterialPropertyType::Float:
                EnsureType<float>(propertyValue);
                break;
            case MaterialPropertyType::Float2:
                EnsureType<float2>(propertyValue);
                break;
            case MaterialPropertyType::Float3:
                EnsureType<float3>(propertyValue);
                break;
            case MaterialPropertyType::Float4:
                EnsureType<float4>(propertyValue);
                break;
            case MaterialPropertyType::Color: {
                EnsureType<Color>(propertyValue);
                break;
            }
            case MaterialPropertyType::Int:
            case MaterialPropertyType::Int2:
            case MaterialPropertyType::Int3:
            case MaterialPropertyType::Int4:
            case MaterialPropertyType::Texture2D:
                EnsureType<AssetRef<Texture2D>>(propertyValue);
                break;
            case MaterialPropertyType::UInt:
            case MaterialPropertyType::UInt2:
            case MaterialPropertyType::UInt3:
            case MaterialPropertyType::UInt4:
            case MaterialPropertyType::Unknown:
                break;
            }
        }
    private:
        template <typename T, typename Variant>
        static void EnsureType(Variant& variant) {
            if (std::holds_alternative<T>(variant)) {
                return;
            }

            variant.template emplace<T>();
        }

    private:
        AssetRef<GraphicsShader> m_Shader;

        Vec<MaterialPropertySpec> m_MaterialProperties;
        Vec<MaterialPropertyValue> m_Values;

    public:
        static const AssetType& GetStaticType() {
            static AssetType assetType = Quelos::GetAssetType<Material>();
            return assetType;
        }

        [[nodiscard]] const AssetType& GetAssetType() const override { return GetStaticType(); }
    };
}
