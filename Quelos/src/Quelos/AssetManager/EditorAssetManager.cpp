#include "qspch.h"
#include "EditorAssetManager.h"

namespace Quelos {
    bool EditorAssetManager::IsAssetHandleValid(const AssetHandle& handle) const {
        if (!handle) {
            return false;
        }

        return m_AssetRegistry.IsAssetHandleValid(handle);
    }

    bool EditorAssetManager::IsAssetLoaded(const AssetHandle& handle) const {
        if (!handle) {
            return false;
        }

        return m_LoadedAssets.contains(handle);
    }

    Ref<Asset> EditorAssetManager::GetAsset(const AssetHandle& handle) const {
        if (!IsAssetHandleValid(handle)) {
            QS_CORE_ERROR_TAG("AssetManager", "Invalid asset handle {}", handle.ToString());
            return nullptr;
        }

        if (IsAssetLoaded(handle)) {
            return m_LoadedAssets.at(handle);
        }

        const AssetMetadata& metadata = m_AssetRegistry.GetAssetMetadata(handle);
        Ref<Asset> asset;// = AssetImporter::ImportAsset(metadata);

        if (!asset) {
            QS_CORE_ERROR_TAG("AssetManager", "Failed to load asset with handle {}", handle.ToString());
            return nullptr;
        }

        return asset;
    }
}
