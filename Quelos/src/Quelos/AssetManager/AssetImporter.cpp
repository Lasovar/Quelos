#include "qspch.h"
#include "AssetImporter.h"

#include "TextureImporter.h"
#include "magic_enum/magic_enum.hpp"

namespace Quelos {
    static HashMap<AssetTypeID, AssetImporterConfig> s_AssetLoaders;

    void AssetImporter::RegisterAssetImporter(const AssetImporterConfig& config) {
        s_AssetLoaders[config.Type] = config;
    }

    bool AssetImporter::ImportAsset(void* data, const AssetMetadata& metadata) {
        QS_PROFILE_SCOPED_N("AssetImporter::ImportAsset");

        const auto it = s_AssetLoaders.find(metadata.Type);
        if (it == s_AssetLoaders.end()) {
            QS_CORE_ERROR_TAG(
                "AssetImport::ImportAsset", "Failed to import asset '{}': No suitable importer",
                metadata.FilePath
            );

            return false;
        }

        return it->second.ImportAsset(data, metadata);
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
