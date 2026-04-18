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
    
    std::optional<AssetHandle> AssetImporter::ReadAssetHandle(const std::string_view assetPath) {
        const AssetType type = GetAssetType(assetPath);
        if (!type) {
            return std::nullopt;
        }
        
        const auto it = s_AssetLoaders.find(type);
        if (it != s_AssetLoaders.end() && it->second.ReadAssetHandle) {
            return it->second.ReadAssetHandle(assetPath);
        }
        
        return std::nullopt;
    }
    
    bool AssetImporter::WriteAssetHandle(const std::string_view assetPath, const AssetHandle& handle) {
        const AssetType type = GetAssetType(assetPath);
        if (!type) {
            return false;
        }
        
        const auto it = s_AssetLoaders.find(type);
        if (it != s_AssetLoaders.end() && it->second.WriteAssetHandle) {
            return it->second.WriteAssetHandle(assetPath, handle);
        }
        
        return false;
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
