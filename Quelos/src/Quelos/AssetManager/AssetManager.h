#pragma once
#include "Asset.h"
#include "Quelos/Core/Application.h"

namespace Quelos {
    class AssetManager {
    public:
        template<typename T>
        [[nodiscard]] static Ref<T> GetAsset(const AssetHandle& handle) {
            Ref<Asset> asset = Application::Get().GetAssetManager()->GetAsset(handle);
            return RefAs<T>(asset);
        }
    };
}
