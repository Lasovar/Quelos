#include "qspch.h"
#include "AssetRegistry.h"

namespace Quelos {
    bool AssetRegistry::IsAssetHandleValid(const AssetID& handle) const {
        return m_AssetMetadata.find(handle) != m_AssetMetadata.end();
    }

    bool AssetRegistry::IsAssetPathValid(const std::string_view path) const {
        return std::ranges::any_of(
            m_AssetMetadata | std::views::values,
            [&path](const AssetMetadata& metadata) { return metadata.FilePath == path; }
        );
    }

    AssetMetadata* AssetRegistry::GetAssetMetadata(const std::string_view path) {
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
    const AssetMetadata* AssetRegistry::GetAssetMetadata(const std::string_view path) const {
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


    AssetMetadata* AssetRegistry::GetAssetMetadata(const AssetID& handle) {
        const auto it = m_AssetMetadata.find(handle);
        if (it == m_AssetMetadata.end()) {
            return nullptr;
        }

        return &it->second;
    }
    const AssetMetadata* AssetRegistry::GetAssetMetadata(const AssetID& handle) const {
        const auto it = m_AssetMetadata.find(handle);
        if (it == m_AssetMetadata.end()) {
            return nullptr;
        }

        return &it->second;
    }

    void AssetRegistry::RemoveAsset(const AssetID assetHandle) {
        const auto it = m_AssetMetadata.find(assetHandle);
        if (it == m_AssetMetadata.end()) {
            return;
        }

        std::erase_if(m_AssetMetadata, [&assetHandle](const Pair<AssetID, AssetMetadata>& metadata) {
            return metadata.second.ParentId == assetHandle;
        });

        m_AssetMetadata.erase(it);
    }
}
