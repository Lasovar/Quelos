#pragma once

#include <ranges>

#include "Asset.h"
#include "AssetPool.h"
#include "Quelos/Core/Application.h"

namespace Quelos {
    namespace AssetManager {
        template<typename T>
        requires std::is_base_of_v<Asset, T>
        static Vec<const AssetMetadata*> FindAssetsOfType() {
            return Project::GetAssetManager()->FindAssetsOfType(T::GetStaticType());
        }

        template <typename T>
        T* TryGet(const AssetHandle<T>& handle) {
            return static_cast<T*>(Project::GetAssetManager()->TryGet(handle));
        }

        template <typename T>
        T& Get(const AssetHandle<T>& handle) {
            T* assetData = static_cast<T*>(Project::GetAssetManager()->TryGet(handle));
            QS_ASSERT(assetData, "Asset not found!");
            return *assetData;
        }
    };
}
