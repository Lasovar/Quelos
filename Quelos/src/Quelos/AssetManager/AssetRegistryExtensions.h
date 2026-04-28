#pragma once

#include "AssetMetadata.h"
#include <functional>

namespace Quelos {
    class AssetManagerBase;

    using RegisterAdditionalAssetsFn = Vec<AssetMetadata>(*)(
        std::string_view assetPath,
        const AssetMetadata& mainAssetMetadata,
        void* userData
    );

    using ResolveSubAssetFn = bool(*)(
        void* dataSlot,
        const AssetMetadata& subAssetMetadata,
        void* userData
    );

    using HandlesAssetTypeFn = bool(*)(const AssetType& type, void* userData);

    struct QS_API AssetRegistryExtensionFunctions {
        RegisterAdditionalAssetsFn RegisterAdditionalAssets;
        ResolveSubAssetFn ResolveSubAsset;
        HandlesAssetTypeFn HandlesAssetType;
        void* UserData;
    };

    class QS_API AssetRegistryExtensions {
    public:
        static void RegisterExtension(const AssetRegistryExtensionFunctions& functions);

        static Vec<AssetMetadata> ProcessAssetRegistration(
            std::string_view assetPath,
            const AssetMetadata& mainAssetMetadata
        );

        static bool ResolveSubAsset(
            void* dataSlot,
            const AssetMetadata& subAssetMetadata
        );
    };

    // Template helper for creating extension function tables
    template<typename T>
    struct AssetRegistryExtension {
        static Vec<AssetMetadata> RegisterAdditionalAssets(
            const std::string_view assetPath,
            const AssetMetadata& mainAssetMetadata,
            void* userData
        ) {
            T* instance = static_cast<T*>(userData);
            return instance->RegisterAdditionalAssets(assetPath, mainAssetMetadata);
        }
        
        static bool ResolveSubAsset(
            void* dataSlot,
            const AssetMetadata& subAssetMetadata,
            void* userData
        ) {
            T* instance = static_cast<T*>(userData);
            return instance->ResolveSubAsset(dataSlot, subAssetMetadata);
        }
        
        static bool HandlesAssetType(const AssetType& type, void* userData) {
            T* instance = static_cast<T*>(userData);
            return instance->HandlesAssetType(type);
        }
        
        static AssetRegistryExtensionFunctions Create(T& instance) {
            return {
                RegisterAdditionalAssets,
                ResolveSubAsset,
                HandlesAssetType,
                &instance
            };
        }
    };
}
