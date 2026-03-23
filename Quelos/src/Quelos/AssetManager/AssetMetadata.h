#pragma once

#include <filesystem>
#include "Asset.h"

namespace Quelos {
    struct AssetMetadata {
        AssetHandle Handle;
        std::filesystem::path FilePath;
        AssetType Type;

        AssetMetadata() = default;
        AssetMetadata(const AssetHandle handle, const Path& path, const AssetType assetType)
            : Handle(handle), FilePath(path), Type(assetType) {}

        operator bool() const { return !Handle || Type != AssetType::None; }
    };
}
