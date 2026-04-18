#pragma once

#include "Quelos/AssetManager/Assets/Model.h"

namespace QuelosEditor {
    using namespace Quelos;

    struct ModelAssetRegistryExtension {
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
            return type == Model::GetStaticType() || type == Mesh::GetStaticType();
        }

    private:
        static Vec<AssetMetadata> RegisterModelSubAssets(
            std::string_view assetPath,
            const AssetMetadata& modelMetadata
        );

        static Ref<Asset> ResolveMeshSubAsset(
            const AssetHandle& meshHandle,
            const AssetMetadata& meshMetadata
        );

        static Ref<Asset> ResolveMaterialSubAsset(
            const AssetHandle& materialHandle,
            const AssetMetadata& materialMetadata
        );
    };
}
