#include "qspch.h"
#include "AssetImporter.h"

#include "TextureImporter.h"
#include "magic_enum/magic_enum.hpp"

namespace Quelos {
    static HashMap<AssetType, AssetImporterConfig> s_AssetLoaders = {
        { AssetType::Texture2D, TextureImporter::GetImporterConfig() }
    };

    void AssetImporter::RegisterAssetImporter(const AssetImporterConfig& config) {
        s_AssetLoaders[config.Type] = config;
    }

    Ref<Asset> AssetImporter::ImportAsset(const AssetHandle assetHandle, const AssetMetadata& metadata) {
        const auto it = s_AssetLoaders.find(metadata.Type);
        if (it == s_AssetLoaders.end()) {
            QS_CORE_ERROR_TAG(
                "AssetImport::ImportAsset", "Failed to import asset '{}': No suitable importer",
                metadata.FilePath.string()
            );

            return nullptr;
        }

        return it->second.LoadAsset(assetHandle, metadata);
    }

    bool AssetImporter::IsAssetSupported(const Path& path) {
        return std::ranges::any_of(s_AssetLoaders | std::views::values,
            [&path](const AssetImporterConfig& importerConfig) {
                return importerConfig.IsAssetSupported(path);
            }
        );
    }

    AssetType AssetImporter::GetAssetType(const Path& path) {
        for (auto & importerConfig : s_AssetLoaders | std::views::values) {
            if (importerConfig.IsAssetSupported(path)) {
                return importerConfig.Type;
            }
        }

        return AssetType::None;
    }
}
