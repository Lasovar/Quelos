#pragma once

#include "Quelos/Core/Base.h"
#include "Quelos/AssetManager/Asset.h"

namespace Quelos {
    class EditorAssetImporter {
    public:
        static AssetType GetAssetType(const Path& path);
    };
}
