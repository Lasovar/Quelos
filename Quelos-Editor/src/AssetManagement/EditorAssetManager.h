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

        [[nodiscard]] const AssetMetadata* GetAssetMetadata(std::string_view path) const;
        [[nodiscard]] const AssetMetadata* GetAssetMetadata(const AssetHandle& assetHandle) const override;

        [[nodiscard]] bool IsAssetHandleValid(const AssetHandle& handle) const override;
        [[nodiscard]] bool IsAssetPathValid(std::string_view path) const;

        [[nodiscard]] Vec<const AssetMetadata*> FindAssetsOfType(const AssetType& type) const override;

        [[nodiscard]] static bool IsAssetSupported(std::string_view path);

        [[nodiscard]] bool IsAssetLoaded(const AssetHandle& handle) const override;
        const AssetMetadata* AddAssetToRegistry(std::string_view assetPath);
        void RemoveAssetFromRegistry(const AssetHandle& assetHandle);
        
        Vec<const AssetMetadata*> ProcessAssetRegistration(std::string_view assetPath);

        void CleanupAssetMap();

        void SerializeAssetRegistry();
        void DeserializeAssetRegistry();
    private:
        AssetRegistry m_AssetRegistry;
        AssetMap m_LoadedAssets;
    };
}
