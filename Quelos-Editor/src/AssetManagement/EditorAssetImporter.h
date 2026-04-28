#pragma once

#include "Quelos/AssetManager/AssetImporter.h"

namespace QuelosEditor {
    using namespace Quelos;

    struct EditorAssetImporterConfig {
        AssetType Type = {};
        AssetLoaderFn LoadAsset;
        IsAssetSupportedFn IsAssetSupported;

        // From here on Editor-only
        using ReimportFn = bool(*)(void* asset, const AssetMetadata& metadata);
        ReimportFn ReimportAsset = nullptr;

        using ReadHandleFn = AssetID(*)(std::string_view assetPath);
        using WriteHandleFn = bool(*)(std::string_view assetPath, const AssetID& handle);
        using CookAssetFn = Buffer(*)(const AssetMetadata& metadata);

        // Path is passed into the Read/Write function as relative path to the Project root
        ReadHandleFn ReadAssetHandle = nullptr;
        WriteHandleFn WriteAssetHandle = nullptr;

        // Optional - the cooked asset will be passed to the AssetLoader
        CookAssetFn CookAsset = nullptr;
    };

    namespace EditorAssetImporter {
        void RegisterAssetImporter(const EditorAssetImporterConfig& config);

        bool IsAssetSupported(std::string_view path);
        const AssetType& GetAssetType(std::string_view path);
        bool ImportAsset(void* dataSlot, const AssetMetadata& metadata);
        
        // Editor-only metadata handling
        AssetID ReadAssetHandle(std::string_view assetPath);
        bool WriteAssetHandle(std::string_view assetPath, const AssetID& handle);
        void TryReimportAsset(void* dataSlot, const AssetMetadata* assetMetadata);
    }
}
