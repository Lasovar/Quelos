#pragma once
#include "Asset.h"
#include "AssetMetadata.h"

namespace Quelos {
    class QS_API AssetRegistry {
    public:
        // TODO: Removing the reserve causes a crash when rehashing... investigate
        AssetRegistry() : m_AssetMetadata(100) {}

        [[nodiscard]] bool IsAssetHandleValid(const AssetHandle& handle) const;
        [[nodiscard]] bool IsAssetPathValid(std::string_view path) const;
        [[nodiscard]] const AssetMetadata* GetAssetMetadata(std::string_view path) const;
        [[nodiscard]] const AssetMetadata* GetAssetMetadata(const AssetHandle& handle) const;

        void RemoveAsset(AssetHandle assetHandle);

        HashMap<AssetHandle, AssetMetadata>& GetAssetsMetadata() {return m_AssetMetadata; }
        [[nodiscard]] const HashMap<AssetHandle, AssetMetadata>& GetAssetsMetadata() const {return m_AssetMetadata; }

    private:
        HashMap<AssetHandle, AssetMetadata> m_AssetMetadata;
    };
}
