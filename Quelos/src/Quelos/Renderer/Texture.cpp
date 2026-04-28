#include "qspch.h"
#include "Texture.h"

#include "Renderer.h"

namespace Quelos {
    uint16_t TextureHandle::GetNativeHandle() const {
        return Renderer::TextureGetNativeHandle(*this);
    }

    Texture2D::Texture2D(const TextureSpecification& spec, Buffer data) {
        m_Handle = Renderer::CreateTexture(spec, std::move(data));
    }

    Texture2D::~Texture2D() {
        Renderer::Destroy(m_Handle);
    }

    Ref<Texture2D> Texture2D::Create(const TextureSpecification& spec) {
        return CreateRef<Texture2D>(Renderer::CreateTexture(spec));
    }

    Ref<Texture2D> Texture2D::Create(const TextureSpecification& spec, Buffer data) {
        return CreateRef<Texture2D>(Renderer::CreateTexture(spec, std::move(data)));
    }

    Ref<Texture2D> Texture2D::Create(const TextureSpecification& spec, const OsPath& texturePath) {
        return CreateRef<Texture2D>(Renderer::CreateTexture(spec, texturePath));
    }

    void Texture2D::Resize(const glm::uvec2& size) const {
        Renderer::TextureResize(m_Handle, size.x, size.y);
    }

    void Texture2D::Resize(const uint32_t width, const uint32_t height) const {
        Renderer::TextureResize(m_Handle, width, height);
    }

    ImageFormat Texture2D::GetFormat() const {
        return Renderer::GetSpecification(m_Handle)->Format;
    }

    uint32_t Texture2D::GetWidth() const {
        return Renderer::GetSpecification(m_Handle)->Width;
    }

    uint32_t Texture2D::GetHeight() const {
        return Renderer::GetSpecification(m_Handle)->Height;
    }

    glm::uvec2 Texture2D::GetSize() const {
        const TextureSpecification* data = Renderer::GetSpecification(m_Handle);
        return {data->Width, data->Height};
    }

    bool Texture2D::IsVFlipped() const {
        return Renderer::TextureIsVFlipped();
    }
}
