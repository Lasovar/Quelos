#pragma once

#include <filesystem>
#include <utility>
#include "Asset.h"

namespace Quelos {
    struct AssetMetadata {
        AssetHandle Handle;
        Path FilePath = {};
        AssetType Type = AssetType::None;

        AssetMetadata() = default;
        AssetMetadata(const AssetHandle handle, Path path, const AssetType assetType)
            : Handle(handle), FilePath(std::move(path)), Type(assetType) {}

        operator bool() const { return !Handle || Type != AssetType::None; }
    };
}
