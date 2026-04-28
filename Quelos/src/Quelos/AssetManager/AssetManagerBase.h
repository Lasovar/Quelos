#pragma once

#include "Asset.h"
#include "AssetMetadata.h"
#include "AssetPool.h"

namespace Quelos {

    using AssetMap = HashMap<AssetID, UntypedAssetHandle>;
    using AssetPools = HashMap<AssetTypeID, UntypedAssetPool>;

    class QS_API AssetManagerBase {
    public:
        [[nodiscard]] virtual const AssetMetadata* GetAssetMetadata(const AssetID& assetId) const = 0;

        virtual bool IsAlive(UntypedAssetHandle handle) = 0;
        virtual void* TryGet(UntypedAssetHandle assetHandle) = 0;

        virtual void UnloadAsset(AssetID assetHandle) = 0;

        [[nodiscard]] virtual bool IsAssetLoaded(const AssetID& assetId) const = 0;
        [[nodiscard]] virtual bool IsAssetHandleValid(const AssetID& assetId) const = 0;

        [[nodiscard]] virtual Vec<const AssetMetadata*> FindAssetsOfType(AssetTypeID assetId) const = 0;

        // Doesn't increment reference count
        virtual UntypedAssetHandle Acquire(AssetID assetId) = 0;
        // Releases the asset without checking the reference count
        virtual void Release(UntypedAssetHandle assetId) = 0;
        virtual void Release(AssetID assetId) = 0;

        virtual void IncRef(UntypedAssetHandle handle) = 0;
        virtual void DecRef(UntypedAssetHandle handle) = 0;

        virtual ~AssetManagerBase() = default;
    };
}
