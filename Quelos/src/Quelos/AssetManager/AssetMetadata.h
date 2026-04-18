#pragma once

#include <utility>
#include "Asset.h"

namespace Quelos {
    struct QS_API AssetMetadata {
        AssetHandle Handle;
        std::string FilePath = {};
        AssetType Type = {};
        AssetHandle ParentHandle = {};
        std::string VirtualPath = {}; // Virtual path within parent (e.g "meshes/0", "materials/diffuse")

        AssetMetadata() = default;

        AssetMetadata(
            const AssetHandle handle,
            std::string path,
            AssetType assetType
        ) : Handle(handle), FilePath(std::move(path)), Type(std::move(assetType)) { }

        AssetMetadata(
            const AssetHandle handle,
            std::string path,
            AssetType assetType,
            const AssetHandle parentHandle, std::string virtualPath = {}
        ) : Handle(handle),
            FilePath(std::move(path)),
            Type(std::move(assetType)),
            ParentHandle(parentHandle),
            VirtualPath(std::move(virtualPath)) { }

        [[nodiscard]] bool IsSubAsset() const { return ParentHandle; }
        [[nodiscard]] bool IsRootAsset() const { return !IsSubAsset(); }

        operator bool() const { return Handle && Type; }
    };
}
