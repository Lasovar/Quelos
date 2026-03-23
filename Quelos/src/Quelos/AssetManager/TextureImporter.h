#pragma once

#include "AssetMetadata.h"
#include "Quelos/Renderer/Texture.h"

namespace Quelos {
    namespace TextureImporter {
        Ref<Texture2D> ImportTexture2D(const AssetMetadata& metadata);
    }
}
