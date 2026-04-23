#pragma once

#include "Quelos/AssetManager/AssetImporter.h"

namespace QuelosEditor {
    using namespace Quelos;

    struct EditorAssetImporterConfig {
        AssetType Type = {};
        AssetLoaderFn LoadAsset;
        IsAssetSupportedFn IsAssetSupported;

        // Editor-only
        using ReimportFn = void(*)(Ref<Asset>& asset, const AssetMetadata& metadata);
        ReimportFn ReimportAsset = nullptr;

        using ReadHandleFn = AssetHandle(*)(std::string_view assetPath);
        using WriteHandleFn = bool(*)(std::string_view assetPath, const AssetHandle& handle);

        ReadHandleFn ReadAssetHandle = nullptr;
        WriteHandleFn WriteAssetHandle = nullptr;
    };

    namespace EditorAssetImporter {
        void RegisterAssetImporter(const EditorAssetImporterConfig& config);

        bool IsAssetSupported(std::string_view path);
        const AssetType& GetAssetType(std::string_view path);
        Ref<Asset> ImportAsset(AssetHandle assetHandle, const AssetMetadata& metadata);
        
        // Editor-only metadata handling
        AssetHandle ReadAssetHandle(std::string_view assetPath);
        bool WriteAssetHandle(std::string_view assetPath, const AssetHandle& handle);
        void TryReimportAsset(Ref<Asset>& asset, const AssetMetadata* assetMetadata);
    }
}
