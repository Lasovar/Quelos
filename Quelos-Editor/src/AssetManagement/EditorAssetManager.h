#pragma once

#include "Quelos/AssetManager/AssetManagerBase.h"
#include "Quelos/AssetManager/AssetRegistry.h"

#include "efsw/FileWatcherGeneric.hpp"

namespace QuelosEditor {
    using namespace Quelos;

    class EditorAssetManager : public AssetManagerBase, public efsw::FileWatchListener {
    public:
        EditorAssetManager();

        [[nodiscard]] Ref<Asset> GetAsset(const AssetHandle& handle) override;
        void UnloadAsset(AssetHandle assetHandle) override;

        [[nodiscard]] const AssetMetadata* GetAssetMetadata(std::string_view path) const;
        [[nodiscard]] const AssetMetadata* GetAssetMetadata(const AssetHandle& assetHandle) const override;

        [[nodiscard]] bool IsAssetHandleValid(const AssetHandle& handle) const override;
        [[nodiscard]] bool IsAssetPathValid(std::string_view path) const;

        [[nodiscard]] Vec<const AssetMetadata*> FindAssetsOfType(const AssetType& type) const override;

        [[nodiscard]] static bool IsAssetSupported(std::string_view path);

        [[nodiscard]] bool IsAssetLoaded(const AssetHandle& handle) const override;
        const AssetMetadata* AddAssetToRegistry(std::string_view assetPath);
        void RemoveAssetFromRegistry(const AssetHandle& assetHandle);
        
        Vec<const AssetMetadata*> ProcessAssetRegistration(std::string_view assetPath);

        void CleanupAssetMap();

        void SerializeAssetRegistry();
        void DeserializeAssetRegistry();

        // File watch listener implementation
        void handleFileAction(efsw::WatchID watchId, const std::string& dir, const std::string& filename,
            efsw::Action action, std::string oldFilename) override;
    private:
        static void ReimportAsset(Ref<Asset>& asset, const AssetMetadata* assetMetadata);

    private:
        AssetRegistry m_AssetRegistry;
        AssetMap m_LoadedAssets;

        efsw::FileWatcher m_FileWatcher;
        HashMap<efsw::WatchID, AssetHandle> m_WatchedAssets;
    };
}
