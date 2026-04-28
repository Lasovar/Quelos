#pragma once

#include "AssetImporter.h"
#include "AssetMetadata.h"
#include "Quelos/Renderer/Texture.h"

namespace Quelos {
    namespace TextureImporter {
        QS_API bool ImportTexture2D(void* dataSlot, const AssetMetadata& metadata);
        QS_API bool IsAssetSupported(std::string_view path);

        inline AssetImporterConfig GetImporterConfig() {
            return {Texture2D::GetStaticType(), ImportTexture2D, IsAssetSupported};
        }
    }
}
