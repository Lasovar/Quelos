#include "qspch.h"
#include "AssetRegistryExtensions.h"
#include "AssetManagerBase.h"

namespace Quelos {
    static constexpr size_t k_MaxExtensions = 32;
    static AssetRegistryExtensionFunctions m_Extensions[k_MaxExtensions] = {};
    static size_t m_ExtensionCount = 0;

    void AssetRegistryExtensions::RegisterExtension(const AssetRegistryExtensionFunctions& functions) {
        if (m_ExtensionCount < k_MaxExtensions) {
            m_Extensions[m_ExtensionCount++] = functions;
        }
    }

    Vec<AssetMetadata> AssetRegistryExtensions::ProcessAssetRegistration(
        const std::string_view assetPath,
        const AssetMetadata& mainAssetMetadata
    ) {
        Vec<AssetMetadata> additionalAssets;
        
        for (size_t i = 0; i < m_ExtensionCount; ++i) {
            const auto& extension = m_Extensions[i];
            if (extension.HandlesAssetType(mainAssetMetadata.Type, extension.UserData)) {
                Vec<AssetMetadata> assets = extension.RegisterAdditionalAssets(
                    assetPath,
                    mainAssetMetadata,
                    extension.UserData
                );

                additionalAssets.insert(
                    additionalAssets.end(),
                    std::make_move_iterator(assets.begin()),
                    std::make_move_iterator(assets.end())
                );
            }
        }
        
        return additionalAssets;
    }

    Ref<Asset> AssetRegistryExtensions::ResolveSubAsset(
        const AssetHandle& subAssetHandle,
        const AssetMetadata& subAssetMetadata
    ) {
        for (size_t i = 0; i < m_ExtensionCount; ++i) {
            const auto& extension = m_Extensions[i];
            if (extension.HandlesAssetType(subAssetMetadata.Type, extension.UserData)) {
                if (Ref<Asset> asset = extension.ResolveSubAsset(subAssetHandle, subAssetMetadata, extension.UserData)) {
                    return asset;
                }
            }
        }
        
        return nullptr;
    }
}
