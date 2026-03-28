#include "qspch.h"
#include "AssetRegistry.h"

namespace Quelos {
    bool AssetRegistry::IsAssetHandleValid(const AssetHandle& handle) const {
        return m_AssetMetadata.find(handle) != m_AssetMetadata.end();
    }

    bool AssetRegistry::IsAssetPathValid(const Path& path) const {
        return std::ranges::any_of(
            m_AssetMetadata | std::views::values,
            [&path](const AssetMetadata& metadata) { return metadata.FilePath == path; }
        );
    }

    const AssetMetadata* AssetRegistry::GetAssetMetadata(const Path& path) const {
        auto values = m_AssetMetadata | std::views::values;
        const auto it = std::ranges::find_if(
            values,
            [&path](const AssetMetadata& metadata) { return metadata.FilePath == path; }
        );

        if (it == values.end()) {
            return nullptr;
        }

        return &*it;
    }

    const AssetMetadata* AssetRegistry::GetAssetMetadata(const AssetHandle& handle) const {
        const auto it = m_AssetMetadata.find(handle);
        if (it == m_AssetMetadata.end()) {
            return nullptr;
        }

        return &it->second;
    }

    void AssetRegistry::RemoveAsset(const AssetHandle assetHandle) {
        m_AssetMetadata.erase(assetHandle);
    }
}
