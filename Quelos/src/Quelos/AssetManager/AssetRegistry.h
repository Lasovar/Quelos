#pragma once
#include "Asset.h"
#include "AssetMetadata.h"

namespace Quelos {
    class AssetRegistry {
    public:
        [[nodiscard]] bool IsAssetHandleValid(const AssetHandle& handle) const;
        [[nodiscard]] const AssetMetadata& GetAssetMetadata(const AssetHandle& handle) const;

        HashMap<AssetHandle, AssetMetadata>& GetAssetsMetadata() {return m_AssetMetadata; }
    private:
        HashMap<AssetHandle, AssetMetadata> m_AssetMetadata;
    };
}
