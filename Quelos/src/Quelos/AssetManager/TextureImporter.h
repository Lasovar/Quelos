#pragma once

#include "AssetImporter.h"
#include "AssetMetadata.h"
#include "Quelos/Renderer/Texture.h"

namespace Quelos {
    namespace TextureImporter {
        Ref<Texture2D> ImportTexture2D(AssetHandle assetHandle, const AssetMetadata& metadata);
        bool IsAssetSupported(const Path& path);

        inline AssetImporterConfig GetImporterConfig() {
            return {AssetType::Texture2D, ImportTexture2D, IsAssetSupported};
        }
    }
}
