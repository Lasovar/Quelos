#pragma once

#include "Asset.h"
#include "Quelos/Core/Application.h"

namespace Quelos {
    namespace AssetManager {
        template<typename T>
        [[nodiscard]] static Ref<T> GetAsset(const AssetHandle& handle) {
            const Ref<Asset> asset = Project::GetAssetManager()->GetAsset(handle);
            return RefAs<T>(asset);
        }
    };
}
