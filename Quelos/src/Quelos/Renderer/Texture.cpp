#include "Texture.h"

#include "Renderer.h"

namespace Quelos {
    uint64_t TextureHandle::GetNativeHandle() const {
        return Renderer::TextureGetNativeHandle(*this);
    }

    Texture::Texture(const TextureSpec& spec, const BufferView data) {
        m_Handle = Renderer::CreateTexture(spec, data);
    }

    Texture::~Texture() {
        Renderer::Destroy(m_Handle);
    }

    void Texture::Resize(const uint2& size) const {
        QS_CORE_ASSERT(
            Renderer::GetSpecification(m_Handle)->Type == TextureType::Texture2D,
            "Incompatible texture type for Texture::Resize(uint2)"
        );

        Renderer::TextureResize(m_Handle, size.x, size.y);
    }

    void Texture::Resize(const uint32_t width, const uint32_t height) const {
        QS_CORE_ASSERT(
            Renderer::GetSpecification(m_Handle)->Type == TextureType::Texture2D,
            "Incompatible texture type for Texture::Resize(uint32_t,uint32_t)"
        );

        Renderer::TextureResize(m_Handle, width, height);
    }

    TextureFormat Texture::GetFormat() const {
        return Renderer::GetSpecification(m_Handle)->Format;
    }

    uint32_t Texture::GetWidth() const {
        return Renderer::GetSpecification(m_Handle)->Width;
    }

    uint32_t Texture::GetHeight() const {
        return Renderer::GetSpecification(m_Handle)->Height;
    }

    uint2 Texture::GetSize() const {
        const TextureSpec* data = Renderer::GetSpecification(m_Handle);
        return {data->Width, data->Height};
    }

    bool Texture::IsVFlipped() const {
        return Renderer::TextureIsVFlipped();
    }

    TextureType Texture::GetType() const {
        return Renderer::GetSpecification(m_Handle)->Type;
    }

    const TextureSpec& Texture::GetSpec() const {
        return *Renderer::GetSpecification(m_Handle);
    }
}
