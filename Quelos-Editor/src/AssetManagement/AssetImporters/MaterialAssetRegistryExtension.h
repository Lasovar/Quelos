#pragma once

#include "Quelos/AssetManager/AssetRegistryExtensions.h"

namespace QuelosEditor {
    using namespace Quelos;

    struct MaterialAssetRegistryExtension {
        static void Register();

        static Vec<AssetMetadata> RegisterAdditionalAssets(
            std::string_view assetPath,
            const AssetMetadata& mainAssetMetadata
        );

        static Ref<Asset> ResolveSubAsset(
            const AssetHandle& subAssetHandle,
            const AssetMetadata& subAssetMetadata
        );

        static bool HandlesAssetType(const AssetType& type) {
            return false/*type == AssetType::Material*/;
        }

    private:
        // Standalone materials don't have sub-assets, so this returns empty
        static Vec<AssetMetadata> RegisterMaterialSubAssets(
            std::string_view assetPath,
            const AssetMetadata& materialMetadata
        );
    };
}
