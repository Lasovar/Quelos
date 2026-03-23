#pragma once

#include "Asset.h"
#include "AssetMetadata.h"

namespace Quelos {
    namespace AssetImporter {
        Ref<Asset> ImportAsset(AssetHandle assetHandle, const AssetMetadata& metadata);
    }
}
