#pragma once
#include "AssetManagerBase.h"
#include "AssetRegistry.h"

namespace Quelos {
    class EditorAssetManager : public AssetManagerBase {
    public:
        [[nodiscard]] Ref<Asset> GetAsset(const AssetHandle& handle) override;
        void UnloadAsset(AssetHandle assetHandle) override;

        [[nodiscard]] const AssetMetadata* GetAssetMetadata(const Path& path) const;
        [[nodiscard]] const AssetMetadata* GetAssetMetadata(const AssetHandle& assetHandle) const;

        [[nodiscard]] bool IsAssetHandleValid(const AssetHandle& handle) const override;
        [[nodiscard]] bool IsAssetPathValid(const Path& path) const;

        [[nodiscard]] static bool IsAssetSupported(const Path& path);

        [[nodiscard]] bool IsAssetLoaded(const AssetHandle& handle) const override;
        const AssetMetadata* AddAssetToRegistry(const Path& assetPath);
        void RemoveAssetFromRegistry(const AssetHandle& assetHandle);

        void CleanupAssetMap();

        void SerializeAssetRegistry();
        void DeserializeAssetRegistry();
    private:
        AssetRegistry m_AssetRegistry;
        AssetMap m_LoadedAssets;

        // TODO: Memory only assets
    };
}
