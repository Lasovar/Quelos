#pragma once

#include <utility>
#include "Asset.h"

namespace Quelos {
    struct QS_API AssetMetadata {
        AssetID Handle;
        std::string FilePath = {};
        AssetType Type = {};
        AssetID ParentId = {};

        AssetMetadata() = default;

        AssetMetadata(
            const AssetID handle,
            std::string path,
            AssetType assetType
        ) : Handle(handle), FilePath(std::move(path)), Type(std::move(assetType)) { }

        AssetMetadata(
            const AssetID handle,
            std::string path,
            AssetType assetType,
            const AssetID parentHandle
        ) : Handle(handle),
            FilePath(std::move(path)),
            Type(std::move(assetType)),
            ParentId(parentHandle)
           { }

        [[nodiscard]] bool IsSubAsset() const { return ParentId; }
        [[nodiscard]] bool IsRootAsset() const { return !IsSubAsset(); }

        operator bool() const { return Handle && Type; }
    };
}
