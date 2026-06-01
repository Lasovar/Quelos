//
// Created by lasovar on 5/28/26.
//

#pragma once

#include <variant>

#include "Quelos/Renderer/Material.h"

namespace QuelosEditor {
    using namespace Quelos;

    struct MaterialProperty {
        MaterialPropertyType Type;
        MaterialPropertyValue Value;

        template <typename TArchive>
        static void Serialize(TArchive& archive, MaterialProperty& materialProperty) {
            archive.Value(materialProperty.Type);

            Material::EnsureType(materialProperty.Type, materialProperty.Value);

            std::visit(
                [&](auto& value) {
                    archive.Value(value);
                },
                materialProperty.Value
            );
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
        AssetID CreateDefaultMaterialAsset(std::string_view materialPath);

        bool IsAssetSupported(std::string_view materialPath);
        bool LoadAsset(void* slot, const AssetMetadata& assetMetadata);
        AssetID ReadAssetID(std::string_view materialPath);
        bool WriteAssetID(std::string_view materialPath, const AssetID& assetId);

        void Initialize();
    }
}
