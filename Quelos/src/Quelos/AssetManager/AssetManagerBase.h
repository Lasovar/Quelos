#pragma once
#include "Asset.h"

namespace Quelos {

    using AssetMap = std::unordered_map<AssetHandle, Ref<Asset>>;

    class AssetManagerBase {
    public:
        [[nodiscard]] virtual Ref<Asset> GetAsset(const AssetHandle& handle) const = 0;

        [[nodiscard]] virtual bool IsAssetLoaded(const AssetHandle& handle) const = 0;
        [[nodiscard]] virtual bool IsAssetHandleValid(const AssetHandle& handle) const = 0;

        virtual ~AssetManagerBase() = default;
    };
}
