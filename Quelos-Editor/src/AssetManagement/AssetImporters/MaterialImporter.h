//
// Created by lasovar on 5/28/26.
//

#pragma once

#include <variant>

#include "Quelos/Renderer/Material.h"

namespace QuelosEditor {
    using namespace Quelos;

    using MaterialPropertyValue = std::variant<float, float2, float3, float4, Color>;
    struct MaterialProperty {
        MaterialPropertyType Type;
        MaterialPropertyValue Value;

        template <typename TArchive>
        static void Serialize(TArchive& archive, MaterialProperty& materialProperty) {
            archive.Value(materialProperty.Type);

            switch (materialProperty.Type) {
            case MaterialPropertyType::Float: {
                archive.Value(EnsureType<float>(materialProperty.Value));
                break;
            }
            case MaterialPropertyType::Float2:
                archive.Value(EnsureType<float2>(materialProperty.Value));
                break;
            case MaterialPropertyType::Float3:
                archive.Value(EnsureType<float3>(materialProperty.Value));
                break;
            case MaterialPropertyType::Float4:
                archive.Value(EnsureType<float4>(materialProperty.Value));
                break;
            case MaterialPropertyType::Color: {
                archive.Value(EnsureType<Color>(materialProperty.Value));
                break;
            }
            case MaterialPropertyType::Int:
            case MaterialPropertyType::Int2:
            case MaterialPropertyType::Int3:
            case MaterialPropertyType::Int4:
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
        static T& EnsureType(Variant& variant) {
            if (auto* value = std::get_if<T>(&variant))
                return *value;

            return variant.template emplace<T>();
        }
    };

    struct MaterialMetadata {
        AssetID AssetId;
        AssetID ShaderId;

        HashMap<std::string, MaterialProperty> Properties;

        template <typename TArchive>
        static void Serialize(TArchive& archive, MaterialMetadata& metadata) {
            archive.Section("Material");
            archive.CloseSection();

            archive.Field("assetId", metadata.AssetId);
            archive.Field("shader", metadata.ShaderId);

            archive.Field("properties", metadata.Properties);
        }
    };

    namespace MaterialImporter {
        void SaveMaterial(const AssetRef<Material>& materialAsset);
        AssetID CreateDefaultMaterialAsset(std::string_view parentDirectory, std::string_view materialName);

        bool IsAssetSupported(std::string_view materialPath);
        bool LoadAsset(void* slot, const AssetMetadata& assetMetadata);
        AssetID ReadAssetID(std::string_view materialPath);
        bool WriteAssetID(std::string_view materialPath, const AssetID& assetId);

        void Initialize();
    }
}
