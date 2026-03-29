#pragma once

#include "Quelos/Core/GUID.h"
#include "Quelos/Core/Ref.h"

namespace Quelos {

    using AssetHandle = GUID128;

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

    class Asset : public RefCounted<Asset> {
    public:
        virtual ~Asset() = default;
        AssetHandle GetAssetHandle() const { return Handle; }
        void SetAssetHandle(const AssetHandle handle) { Handle = handle; }

        static AssetType GetStaticType() { return AssetType::None; }
        virtual AssetType GetAssetType() const = 0;
    protected:
        AssetHandle Handle;
    };
}
