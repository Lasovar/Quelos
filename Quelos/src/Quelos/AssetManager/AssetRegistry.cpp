#include "qspch.h"
#include "AssetRegistry.h"

namespace Quelos {
    bool AssetRegistry::IsAssetHandleValid(const AssetHandle& handle) const {
        return m_AssetMetadata.contains(handle);
    }

    const AssetMetadata& AssetRegistry::GetAssetMetadata(const AssetHandle& handle) const {
        static AssetMetadata s_NullAssetMetadata;
        const auto it = m_AssetMetadata.find(handle);
        if (it == m_AssetMetadata.end()) {
            return s_NullAssetMetadata;
        }

        return it->second;
    }
}
