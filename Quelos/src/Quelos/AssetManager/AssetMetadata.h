#pragma once

#include <utility>
#include "Asset.h"

namespace Quelos {
    struct QS_API AssetMetadata {
        AssetID ID;
        std::string FilePath = {};
        AssetType Type = {};
        AssetID ParentID = {};

        AssetMetadata() = default;

        AssetMetadata(
            const AssetID handle,
            std::string path,
            AssetType assetType
        ) : ID(handle), FilePath(std::move(path)), Type(std::move(assetType)) { }

        AssetMetadata(
            const AssetID handle,
            std::string path,
            AssetType assetType,
            const AssetID parentHandle
        ) : ID(handle),
            FilePath(std::move(path)),
            Type(std::move(assetType)),
            ParentID(parentHandle)
           { }

        [[nodiscard]] bool IsSubAsset() const { return ParentID; }
        [[nodiscard]] bool IsRootAsset() const { return !IsSubAsset(); }

        operator bool() const { return ID && Type; }
    };
}
