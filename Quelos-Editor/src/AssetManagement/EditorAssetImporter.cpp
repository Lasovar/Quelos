#include "qspch.h"
#include "EditorAssetImporter.h"

namespace QuelosEditor {
    using namespace Quelos;

    static HashMap<AssetType, EditorAssetImporterConfig> s_EditorAssetLoaders;

    void EditorAssetImporter::RegisterAssetImporter(const EditorAssetImporterConfig& config) {
        s_EditorAssetLoaders[config.Type] = config;
        
        // Also register with runtime AssetImporter for game builds
        const AssetImporterConfig runtimeConfig = {
            config.Type,
            config.LoadAsset,
            config.IsAssetSupported
        };

        AssetImporter::RegisterAssetImporter(runtimeConfig);
    }

    bool EditorAssetImporter::IsAssetSupported(std::string_view path) {
        if (!std::ranges::any_of(s_EditorAssetLoaders | std::views::values,
            [&path](const EditorAssetImporterConfig& importerConfig) {
                return importerConfig.IsAssetSupported(path);
            }
        )) {
            return AssetImporter::IsAssetSupported(path);
        }

        return true;
    }

    const AssetType& EditorAssetImporter::GetAssetType(const std::string_view path) {
        for (auto& importerConfig : s_EditorAssetLoaders | std::views::values) {
            if (importerConfig.IsAssetSupported(path)) {
                return importerConfig.Type;
            }
        }

        return AssetImporter::GetAssetType(path);
    }

    bool EditorAssetImporter::ImportAsset(void* dataSlot, const AssetMetadata& metadata) {
        const auto it = s_EditorAssetLoaders.find(metadata.Type);
        if (it == s_EditorAssetLoaders.end()) {
            return AssetImporter::ImportAsset(dataSlot, metadata);
        }

        if (it->second.CookAsset) {
            if (!it->second.CookAsset(metadata)) {
                return false;
            }
        }

        return it->second.LoadAsset(dataSlot, metadata);
    }

    AssetID EditorAssetImporter::ReadAssetHandle(const std::string_view assetPath) {
        const AssetType type = GetAssetType(assetPath);
        if (!type) {
            return {};
        }
        
        const auto it = s_EditorAssetLoaders.find(type);
        if (it != s_EditorAssetLoaders.end() && it->second.ReadAssetHandle) {
            return it->second.ReadAssetHandle(assetPath);
        }
        
        return {};
    }

    bool EditorAssetImporter::WriteAssetHandle(std::string_view assetPath, const AssetID& handle) {
        const AssetType type = GetAssetType(assetPath);
        if (!type) {
            return false;
        }
        
        const auto it = s_EditorAssetLoaders.find(type);
        if (it != s_EditorAssetLoaders.end() && it->second.WriteAssetHandle) {
            return it->second.WriteAssetHandle(assetPath, handle);
        }
        
        return false;
    }

    void EditorAssetImporter::TryReimportAsset(void* dataSlot, const AssetMetadata* assetMetadata) {
        if (!assetMetadata) {
            return;
        }

        if (const auto it = s_EditorAssetLoaders.find(assetMetadata->Type); it != s_EditorAssetLoaders.end()) {
            if (it->second.CookAsset) {
                if (!it->second.CookAsset(*assetMetadata)) {
                    return;
                }
            }

            if (it->second.ReimportAsset) {
                it->second.ReimportAsset(dataSlot, *assetMetadata);
            }
        }
    }
}
