#pragma once

#include <filesystem>
#include <utility>
#include "Asset.h"

namespace Quelos {
    struct QS_API AssetMetadata {
        AssetHandle Handle;
        std::string FilePath = {};
        AssetType Type = AssetType::None;

        AssetMetadata() = default;
        AssetMetadata(const AssetHandle handle, std::string path, const AssetType assetType)
            : Handle(handle), FilePath(std::move(path)), Type(assetType) {}

        operator bool() const { return !Handle || Type != AssetType::None; }
    };
}
