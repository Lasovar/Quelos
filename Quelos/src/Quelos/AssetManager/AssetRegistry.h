#pragma once
#include "Asset.h"
#include "AssetMetadata.h"

namespace Quelos {
    class QS_API AssetRegistry {
    public:
        [[nodiscard]] bool IsAssetHandleValid(const AssetHandle& handle) const;
        [[nodiscard]] bool IsAssetPathValid(const Path& path) const;
        [[nodiscard]] const AssetMetadata* GetAssetMetadata(const Path& path) const;
        [[nodiscard]] const AssetMetadata* GetAssetMetadata(const AssetHandle& handle) const;

        void RemoveAsset(AssetHandle assetHandle);

        HashMap<AssetHandle, AssetMetadata>& GetAssetsMetadata() {return m_AssetMetadata; }

    private:
        HashMap<AssetHandle, AssetMetadata> m_AssetMetadata;
    };
}
