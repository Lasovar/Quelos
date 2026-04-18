#pragma once

#include "AssetImporter.h"
#include "AssetMetadata.h"
#include "Quelos/Renderer/Texture.h"

namespace Quelos {
    namespace TextureImporter {
        QS_API Ref<Texture2D> ImportTexture2D(AssetHandle assetHandle, const AssetMetadata& metadata);
        QS_API bool IsAssetSupported(std::string_view path);

        inline AssetImporterConfig GetImporterConfig() {
            return {Texture2D::GetStaticType(), ImportTexture2D, IsAssetSupported};
        }
    }
}
