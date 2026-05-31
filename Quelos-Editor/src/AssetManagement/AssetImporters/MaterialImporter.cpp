//
// Created by lasovar on 5/28/26.
//

#include "MaterialImporter.h"

#include "AssetManagement/EditorAssetImporter.h"
#include "AssetManagement/EditorAssetManager.h"
#include "magic_enum/magic_enum.hpp"
#include "Quelos/Serialization/QuelArchive.h"
#include "Quelos/Serialization/Serializer.h"
#include "Quelos/Utility/FileSystem.h"

namespace QuelosEditor {
    namespace MaterialImporter {
        const char* k_MaterialExtension = ".qmat";

        OsPath GetMetadataPath(const std::string_view assetDirectory, const std::string_view assetName) {
            return Project::GetProjectPath() / assetDirectory / assetName;
        }

        OsPath GetMetadataPath(const std::string_view assetPath) {
            return Project::GetProjectPath() / assetPath;
        }

        bool SerializeMaterial(const OsPath& materialPath, MaterialMetadata& metadata) {
            using namespace Serialization;

            std::ofstream materialFile(materialPath, std::ios::binary);
            if (!materialFile) {
                return false;
            }

            std::string buffer;
            StringQuelWriter writer(buffer);
            QuelWriteArchive writeArchive(writer);
            MaterialMetadata::Serialize(writeArchive, metadata);

            materialFile.write(buffer.data(), buffer.size());

            return true;
        }

        Option<MaterialMetadata> DeserializeMaterial(const OsPath& materialPath) {
            using namespace Serialization;

            std::ifstream materialFile(materialPath, std::ios::binary | std::ios::ate);
            if (!materialFile) {
                return None;
            }

            size_t size = materialFile.tellg();
            materialFile.seekg(0);

            std::string buffer;
            buffer.resize(size);

            materialFile.read(buffer.data(), size);

            MaterialMetadata materialMetadata;

            QuelReader writer(buffer);
            QuelAutoReadArchive writeArchive(writer);
            MaterialMetadata::Serialize(writeArchive, materialMetadata);

            return materialMetadata;
        }

        void SaveMaterial(const AssetRef<Material>& materialAsset) {
            if (!materialAsset) {
                return;
            }

            AssetID assetId = materialAsset.GetAssetID();
            if (!assetId) {
                assetId = AssetID::Generate();
                materialAsset->SetAssetID(assetId);
            }

            Material& material = materialAsset.Get();
            MaterialMetadata materialMetadata;
            materialMetadata.AssetId = assetId;
            materialMetadata.ShaderId = material.GetShader().GetAssetID();

            const Vec<MaterialPropertySpec>& specs = material.GetMaterialProperties();
            const Vec<MaterialPropertyValue>& values = material.GetMaterialPropertyValues();

            for (uint32_t i = 0; i < specs.size(); i++) {
                const MaterialPropertySpec& materialProperty = specs[i];
                MaterialProperty property;
                property.Type = materialProperty.Type;
                property.Value = values[i];
                materialMetadata.Properties[materialProperty.Name] = property;
            }

            SerializeMaterial(
                GetMetadataPath(Project::GetAssetManager()->GetAssetMetadata(assetId)->FilePath),
                materialMetadata
            );
        }

        AssetID CreateDefaultMaterialAsset(const std::string_view parentDirectory, const std::string_view materialName) {
            const OsPath parentDirectoryPath(parentDirectory);
            if (!std::filesystem::exists(parentDirectoryPath)) {
                std::filesystem::create_directories(parentDirectoryPath);
            }

            const OsPath materialPath = GetMetadataPath(parentDirectory, materialName);

            MaterialMetadata materialMetadata;
            materialMetadata.AssetId = AssetID::Generate();
            materialMetadata.ShaderId = {};

            if (!SerializeMaterial(materialPath, materialMetadata)) {
                return {};
            }

            return materialMetadata.AssetId;
        }

        bool IsAssetSupported(const std::string_view path) {
            return FS::Extension(path) == k_MaterialExtension;
        }

        bool LoadAsset(void* slot, const AssetMetadata& assetMetadata) {
            Option<MaterialMetadata> materialMetadataResult = DeserializeMaterial(GetMetadataPath(assetMetadata.FilePath));
            if (!materialMetadataResult) {
                return false;
            }

            MaterialMetadata& materialMetadata = *materialMetadataResult;

            Material* material = new(slot) Material();
            material->SetAssetID(assetMetadata.Handle);
            material->SetShader(materialMetadata.ShaderId);

            for (const MaterialPropertySpec& materialProperty : material->GetMaterialProperties()) {
                auto it = materialMetadata.Properties.find(materialProperty.Name);
                if (it == materialMetadata.Properties.end()) {
                    continue;
                }

                const auto& propertyValue = it->second;
                std::visit(
                    [&](const auto& value) {
                        material->SetProperty(materialProperty.Offset, value);
                    },
                    propertyValue.Value
                );
            }


            return true;
        }

        AssetID ReadAssetID(const std::string_view materialPath) {
            const Option<MaterialMetadata> materialMetadata = DeserializeMaterial(GetMetadataPath(materialPath));
            if (!materialMetadata) {
                return {};
            }

            return materialMetadata->AssetId;
        }

        bool WriteAssetID(const std::string_view materialPath, const AssetID& assetId) {
            const OsPath metadataPath = GetMetadataPath(materialPath);
            Option<MaterialMetadata> materialMetadata = DeserializeMaterial(metadataPath);
            if (materialMetadata && materialMetadata->AssetId == assetId) {
                return true;
            }

            if (!materialMetadata) {
                materialMetadata.emplace();
            }

            materialMetadata->AssetId = assetId;
            return SerializeMaterial(metadataPath, *materialMetadata);
        }

        EditorAssetImporterConfig GetImporterConfig() {
            return {
                Material::GetStaticType(),
                LoadAsset,
                IsAssetSupported,
                nullptr,
                ReadAssetID,
                WriteAssetID
            };
        }

        void Initialize() {
            EditorAssetImporter::RegisterAssetImporter(GetImporterConfig());
        }
    }
}
