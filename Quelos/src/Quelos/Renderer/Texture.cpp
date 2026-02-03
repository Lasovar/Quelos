#include "qspch.h"
#include "Texture.h"

#include "Quelos/Platform/bgfx/bgfxTexture.h"

namespace Quelos {
    Ref<Texture2D> Texture2D::Create(const TextureSpecification& spec) {
        return CreateRef<bgfxTexture2D>(spec);
    }

    Ref<Texture2D> Texture2D::Create(const TextureSpecification& spec, const std::filesystem::path& texturePath) {
        return CreateRef<bgfxTexture2D>(spec, texturePath);
    }
}
