#include "qspch.h"
#include "EditorAssetImporter.h"

#include "SidecarHandler.hpp"
#include "Quelos/Scenes/Scene.h"

namespace QuelosEditor {
    using namespace Quelos;

    static HashMap<AssetType, EditorAssetImporterConfig> s_EditorAssetLoaders{Allocator::Persistent};
    static HashSet<AssetTypeID> s_NoSidecar{{GetAssetTypeID<Scene>()}, Allocator::Persistent};

    void EditorAssetImporter::RegisterAssetImporter(const EditorAssetImporterConfig& config) {
        s_EditorAssetLoaders[config.Type] = config;

        if (config.ReadAssetHandle && config.WriteAssetHandle) {
            s_NoSidecar.emplace(config.Type);
        }

        // Also register with runtime AssetImporter for game builds
        const AssetImporterConfig runtimeConfig = {
            .Type = config.Type,
            .LoadAsset = config.LoadAsset,
            .IsAssetSupported = config.IsAssetSupported
        };

        AssetImporter::RegisterAssetImporter(runtimeConfig);
    }

    bool EditorAssetImporter::IsUsingSidecar(const AssetTypeID assetTypeId) {
        return !s_NoSidecar.contains(assetTypeId);
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

    bool EditorAssetImporter::LoadAsset(const LoadContext& loadContext) {
        const auto it = s_EditorAssetLoaders.find(loadContext.GetAssetMetadata().Type);
        if (it == s_EditorAssetLoaders.end()) {
            return AssetImporter::LoadAsset(loadContext);
        }

        return it->second.LoadAsset(loadContext);
    }

    AssetID EditorAssetImporter::ReadAssetHandle(const std::string_view assetPath) {
        const AssetType type = GetAssetType(assetPath);
        if (!type) {
            return {};
        }

        if (!s_NoSidecar.contains(type)) {
            return SidecarHandler::ReadAssetID(type.GetName(), assetPath).value_or({});
        }

        const auto it = s_EditorAssetLoaders.find(type);
        if (it != s_EditorAssetLoaders.end() && it->second.ReadAssetHandle) {
            return it->second.ReadAssetHandle(assetPath);
        }

        return {};
    }

    bool EditorAssetImporter::WriteAssetHandle(const std::string_view assetPath, const AssetID& handle) {
        const AssetType type = GetAssetType(assetPath);
        if (!type) {
            return false;
        }

        if (!s_NoSidecar.contains(type)) {
            if (auto r = SidecarHandler::WriteAssetID(type.GetName(), assetPath, handle); !r) {
                QS_ERROR_TAGGED("Failed to write asset handle: {}", r.error());
                return false;
            }

            return true;
        }

        const auto it = s_EditorAssetLoaders.find(type);
        if (it != s_EditorAssetLoaders.end() && it->second.WriteAssetHandle) {
            return it->second.WriteAssetHandle(assetPath, handle);
        }

        return false;
    }

    bool EditorAssetImporter::Import(ImportContext& importContext) {
        const AssetMetadata& metadata = importContext.GetAssetMetadata();
        if (const auto it = s_EditorAssetLoaders.find(metadata.Type); it != s_EditorAssetLoaders.end()) {
            if (!it->second.ImportAsset) {
                return true;
            }

            if (!it->second.ImportAsset(importContext)) {
                return false;
            }
        }

        return true;
    }

    Vec<AssetDependency> EditorAssetImporter::GetAssetDependencies(
        const AssetMetadata& assetMetadata, std::pmr::memory_resource* memoryResource
    ) {
        if (const auto it = s_EditorAssetLoaders.find(assetMetadata.Type); it != s_EditorAssetLoaders.end()) {
            if (it->second.GetDependencies) {
                return it->second.GetDependencies(assetMetadata, memoryResource);
            }
        }

        return {};
    }
}
