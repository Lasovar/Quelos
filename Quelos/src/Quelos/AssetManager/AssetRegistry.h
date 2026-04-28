#pragma once
#include "Asset.h"
#include "AssetMetadata.h"

namespace Quelos {
    class QS_API AssetRegistry {
    public:
        // TODO: Removing the reserve causes a crash when rehashing... investigate
        AssetRegistry() : m_AssetMetadata(100) {}

        [[nodiscard]] bool IsAssetHandleValid(const AssetID& handle) const;
        [[nodiscard]] bool IsAssetPathValid(std::string_view path) const;
        AssetMetadata* GetAssetMetadata(std::string_view path);
        [[nodiscard]] const AssetMetadata* GetAssetMetadata(std::string_view path) const;
        AssetMetadata* GetAssetMetadata(const AssetID& handle);
        [[nodiscard]] const AssetMetadata* GetAssetMetadata(const AssetID& handle) const;

        void RemoveAsset(AssetID assetHandle);

        HashMap<AssetID, AssetMetadata>& GetAssetsMetadata() {return m_AssetMetadata; }
        [[nodiscard]] const HashMap<AssetID, AssetMetadata>& GetAssetsMetadata() const {return m_AssetMetadata; }

    private:
        HashMap<AssetID, AssetMetadata> m_AssetMetadata;
    };
}
