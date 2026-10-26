#pragma once

#include "AssetImporter.h"
#include "AssetMetadata.h"
#include "Quelos/Renderer/Texture.h"

namespace Quelos {
    namespace TextureImporter {
        QS_API bool ImportTexture2D(const LoadContext& loadContext);
        QS_API bool IsAssetSupported(std::string_view path);

        inline AssetImporterConfig GetImporterConfig() {
            return {Texture::GetStaticType(), ImportTexture2D, IsAssetSupported};
        }
    }
}
