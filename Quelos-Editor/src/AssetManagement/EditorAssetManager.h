#pragma once

#include "Quelos/AssetManager/AssetManagerBase.h"
#include "Quelos/AssetManager/AssetRegistry.h"

namespace QuelosEditor {
    using namespace Quelos;

    class EditorAssetManager : public AssetManagerBase {
    public:
        EditorAssetManager();

        [[nodiscard]] Ref<Asset> GetAsset(const AssetHandle& handle) override;
        void UnloadAsset(AssetHandle assetHandle) override;

        [[nodiscard]] const AssetMetadata* GetAssetMetadata(const Path& path) const;
        [[nodiscard]] const AssetMetadata* GetAssetMetadata(const AssetHandle& assetHandle) const override;

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
    };
}
