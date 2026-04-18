#pragma once

#include <ranges>

#include "Asset.h"
#include "Quelos/Core/Application.h"

namespace Quelos {
    namespace AssetManager {
        template<typename T>
        [[nodiscard]] static Ref<T> GetAsset(const AssetHandle& handle) {
            const Ref<Asset> asset = Project::GetAssetManager()->GetAsset(handle);
            return RefAs<T>(asset);
        }

        template<typename T>
        requires std::is_base_of_v<Asset, T>
        static Vec<const AssetMetadata*> FindAssetsOfType() {
            return Project::GetAssetManager()->FindAssetsOfType(T::GetStaticType());
        }
    };
}
