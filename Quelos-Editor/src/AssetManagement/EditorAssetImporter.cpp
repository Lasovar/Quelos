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
        return std::ranges::any_of(s_EditorAssetLoaders | std::views::values,
            [&path](const EditorAssetImporterConfig& importerConfig) {
                return importerConfig.IsAssetSupported(path);
            }
        );
    }

    const AssetType& EditorAssetImporter::GetAssetType(const std::string_view path) {
        for (auto & importerConfig : s_EditorAssetLoaders | std::views::values) {
            if (importerConfig.IsAssetSupported(path)) {
                return importerConfig.Type;
            }
        }

        return AssetType::Invalid;
    }

    Ref<Asset> EditorAssetImporter::ImportAsset(AssetHandle assetHandle, const AssetMetadata& metadata) {
        const auto it = s_EditorAssetLoaders.find(metadata.Type);
        if (it == s_EditorAssetLoaders.end()) {
            QS_CORE_ERROR_TAG(
                "EditorAssetImport::ImportAsset", "Failed to import asset '{}': No suitable importer",
                metadata.FilePath
            );

            return nullptr;
        }

        return it->second.LoadAsset(assetHandle, metadata);
    }

    AssetHandle EditorAssetImporter::ReadAssetHandle(std::string_view assetPath) {
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

    bool EditorAssetImporter::WriteAssetHandle(std::string_view assetPath, const AssetHandle& handle) {
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
}
