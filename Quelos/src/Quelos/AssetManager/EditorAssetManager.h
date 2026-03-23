#pragma once
#include "AssetManagerBase.h"
#include "AssetRegistry.h"

namespace Quelos {
    class EditorAssetManager : public AssetManagerBase {
    public:
        [[nodiscard]] Ref<Asset> GetAsset(const AssetHandle& handle) override;

        [[nodiscard]] bool IsAssetLoaded(const AssetHandle& handle) const override;
        AssetHandle AddAssetToRegistry(AssetType assetType, const Path& assetPath);
        [[nodiscard]] bool IsAssetHandleValid(const AssetHandle& handle) const override;

        void SerializeAssetRegistry();
        void DeserializeAssetRegistry();
    private:
        AssetRegistry m_AssetRegistry;
        AssetMap m_LoadedAssets;

        // TODO: Memory only assets
    };
}
