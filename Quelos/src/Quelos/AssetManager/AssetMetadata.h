#pragma once

#include <filesystem>
#include "Asset.h"

namespace Quelos {
    struct AssetMetadata {
        AssetHandle Handle;
        std::filesystem::path FilePath;
        AssetType Type;

        operator bool() const { return !Handle || Type != AssetType::None; }
    };
}
