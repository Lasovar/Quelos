#include "qspch.h"
#include "MaterialAssetRegistryExtension.h"

namespace QuelosEditor {
    using namespace Quelos;

    static MaterialAssetRegistryExtension s_MaterialExtension;

    void MaterialAssetRegistryExtension::Register() {
        AssetRegistryExtensions::RegisterExtension(
            AssetRegistryExtension<MaterialAssetRegistryExtension>::Create(s_MaterialExtension)
        );
    }

    Vec<AssetMetadata> MaterialAssetRegistryExtension::RegisterAdditionalAssets(
        const std::string_view assetPath,
        const AssetMetadata& mainAssetMetadata
    ) {
        return {};
        /*
        if (mainAssetMetadata.Type != AssetType::Material) {
            return {};
        }

        return RegisterMaterialSubAssets(assetPath, mainAssetMetadata);
        */
    }

    Ref<Asset> MaterialAssetRegistryExtension::ResolveSubAsset(
        const AssetHandle& subAssetHandle,
        const AssetMetadata& subAssetMetadata
    ) {
        // Standalone materials are not sub-assets, so this shouldn't be called
        // If it is called, it means this material is a sub-asset of something else
        // (e.g., a material that's part of a model)
        
        if (subAssetMetadata.IsSubAsset()) {
            // This material is a sub-asset of a model or other asset
            // For now, we don't handle this case, but in the future this could
            // load the parent asset and extract the material
            QS_CORE_WARN_TAG("MaterialAssetRegistryExtension", 
                           "Material sub-assets are not yet supported: {}", 
                           subAssetHandle.ToString());
        }
        
        return nullptr;
    }

    Vec<AssetMetadata> MaterialAssetRegistryExtension::RegisterMaterialSubAssets(
        const std::string_view assetPath,
        const AssetMetadata& materialMetadata
    ) {
        // Standalone materials don't have sub-assets
        // This method exists for future extensions where materials might have
        // sub-assets like textures, shaders, etc.
        return {};
    }
}
