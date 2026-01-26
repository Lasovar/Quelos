#pragma once

#include "bgfx/bgfx.h"

#include "Quelos/Renderer/Texture.h"

namespace Quelos {
    class bgfxTexture2D : public Texture2D {
    public:
        bgfxTexture2D(const TextureSpecification& spec, const std::filesystem::path& path);
    private:
        TextureSpecification m_Specification;
        std::filesystem::path m_Path;

        bgfx::TextureHandle m_Handle;
    };
}
