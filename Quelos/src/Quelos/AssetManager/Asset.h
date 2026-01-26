#pragma once

#include "Quelos/Core/GUID.h"

namespace Quelos {

    using AssetHandle = GUID;

    enum class AssetType {
        None = 0,
        Mesh,
        Texture,
        Texture2D,
        Shader,
        Material,
        Scene,
        Script,
        Text,
        Audio,
        Other
    };

    class Asset : RefCounted {
    public:
        AssetHandle GetHandle() const { return Handle; }

        static AssetType GetStaticType() { return AssetType::None; }
        virtual AssetType GetAssetType() const = 0;
    protected:
        AssetHandle Handle;
    };
}
