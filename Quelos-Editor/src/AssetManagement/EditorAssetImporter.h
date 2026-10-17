#pragma once

#include <variant>

#include "ImportContext.hpp"
#include "Quelos/AssetManager/AssetImporter.h"
#include "Quelos/Utility/String.hpp"

namespace QuelosEditor {
    using namespace Quelos;

    using AssetDependency = std::variant<AssetID, String>;

    struct EditorAssetImporterConfig {
        AssetType Type = {};
        AssetLoaderFn LoadAsset;
        IsAssetSupportedFn IsAssetSupported;

        // From here on Editor-only
        using ReadHandleFn = AssetID(*)(std::string_view assetPath);
        using WriteHandleFn = bool(*)(std::string_view assetPath, const AssetID& handle);
        using ImportAssetFn = bool(*)(ImportContext& importContext);
        using AssetHashFn = uint64_t(*)(const AssetMetadata& metadata);
        using GetDependenciesFn = Vec<AssetDependency>(*)(const AssetMetadata& assetMetadata, std::pmr::memory_resource* memoryResource);

        // Path is passed into the Read/Write function as relative path to the Project root
        ReadHandleFn ReadAssetHandle = nullptr;
        WriteHandleFn WriteAssetHandle = nullptr;

        // Used for reimporting
        //AssetHashFn AssetHash = nullptr;

        // Optional - cooked asset should be stored in the Project::CookedAssetsPath and loaded on LoadAsset
        ImportAssetFn ImportAsset = nullptr;

        // Optional
        GetDependenciesFn GetDependencies = nullptr;
    };

    namespace EditorAssetImporter {
        void RegisterAssetImporter(const EditorAssetImporterConfig& config);

        bool IsAssetSupported(std::string_view path);
        const AssetType& GetAssetType(std::string_view path);
        bool LoadAsset(const LoadContext& loadContext);
        
        // Editor-only metadata handling
        AssetID ReadAssetHandle(std::string_view assetPath);
        bool WriteAssetHandle(std::string_view assetPath, const AssetID& handle);
        bool Import(ImportContext& importContext);

        Vec<AssetDependency> GetAssetDependencies(const AssetMetadata& assetMetadata, std::pmr::memory_resource* memoryResource);
    }
}
