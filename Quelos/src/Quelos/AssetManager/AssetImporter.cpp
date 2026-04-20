#include "qspch.h"
#include "AssetImporter.h"

#include "TextureImporter.h"
#include "magic_enum/magic_enum.hpp"

namespace Quelos {
    static HashMap<AssetType, AssetImporterConfig> s_AssetLoaders = {
        { Texture2D::GetStaticType(), TextureImporter::GetImporterConfig() }
    };

    void AssetImporter::RegisterAssetImporter(const AssetImporterConfig& config) {
        s_AssetLoaders[config.Type] = config;
    }

    Ref<Asset> AssetImporter::ImportAsset(const AssetHandle assetHandle, const AssetMetadata& metadata) {
        const auto it = s_AssetLoaders.find(metadata.Type);
        if (it == s_AssetLoaders.end()) {
            QS_CORE_ERROR_TAG(
                "AssetImport::ImportAsset", "Failed to import asset '{}': No suitable importer",
                metadata.FilePath
            );

            return nullptr;
        }

        return it->second.LoadAsset(assetHandle, metadata);
    }

    bool AssetImporter::IsAssetSupported(const std::string_view path) {
        return std::ranges::any_of(s_AssetLoaders | std::views::values,
            [&path](const AssetImporterConfig& importerConfig) {
                return importerConfig.IsAssetSupported(path);
            }
        );
    }

    const AssetType& AssetImporter::GetAssetType(const std::string_view path) {
        for (auto & importerConfig : s_AssetLoaders | std::views::values) {
            if (importerConfig.IsAssetSupported(path)) {
                return importerConfig.Type;
            }
        }

        return AssetType::Invalid;
    }
}
