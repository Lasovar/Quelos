#pragma once

#include "bgfx/bgfx.h"

#include "Quelos/Renderer/Texture.h"

namespace Quelos {
    class bgfxTexture2D : public Texture2D {
    public:
        bgfxTexture2D(const TextureSpecification& spec, const std::filesystem::path& path);

        void Bind(uint32_t slot) const override;

        ImageFormat GetFormat() const override;

        uint32_t GetWidth() const override { return m_Specification.Width; }
        uint32_t GetHeight() const override { return m_Specification.Height; }
        glm::uvec2 GetSize() const override { return { GetWidth(), GetHeight() }; }

        const std::filesystem::path& GetPath() const override { return m_Path; }

        void Resize(const glm::uvec2& size) override;
        void Resize(uint32_t width, uint32_t height) override;

        void CreateFromFile(const TextureSpecification& specification, const std::filesystem::path& filepath) override;

        const void* GetNativeHandle() const override { return &m_Handle; }

    private:
        TextureSpecification m_Specification;
        std::filesystem::path m_Path;

        bgfx::TextureHandle m_Handle = BGFX_INVALID_HANDLE;
    };
}
