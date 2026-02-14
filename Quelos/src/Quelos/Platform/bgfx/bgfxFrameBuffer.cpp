#include "qspch.h"
#include "bgfxFrameBuffer.h"

#include "bgfxTexture.h"

namespace Quelos {
    static bgfx::FrameBufferHandle CreateFrameBufferHandle(const std::vector<Ref<Texture2D>>& attachments) {
        std::vector<bgfx::TextureHandle> bgfxAttachments;
        bgfxAttachments.reserve(attachments.size());

        for (const auto& attachment : attachments) {
            const auto bgfxTexture = RefAs<bgfxTexture2D>(attachment);
            QS_CORE_ASSERT(bgfxTexture, "Attachment is not a bgfxTexture2D!");

            bgfxAttachments.push_back(std::bit_cast<bgfx::TextureHandle>(bgfxTexture->GetTextureHandle()));
        }

        return bgfx::createFrameBuffer(
            static_cast<uint8_t>(bgfxAttachments.size()),
            bgfxAttachments.data(),
            false
        );
    }
    bgfxFrameBuffer::bgfxFrameBuffer(const uint32_t viewID, const std::vector<Ref<Texture2D>>& attachments)
        : m_ViewId(viewID)
    {
        if (attachments.empty()) {
            QS_CORE_ERROR("Creating a FrameBuffer with no attachments!");
            return;
        }

        m_Attachments = attachments;

        const Ref<Texture2D> tex = m_Attachments[0];
        m_Width = tex->GetWidth();
        m_Height = tex->GetHeight();

        m_FrameBufferHandle = CreateFrameBufferHandle(attachments);

        QS_CORE_ASSERT(bgfx::isValid(m_FrameBufferHandle), "Failed to create bgfx FrameBuffer!");
    }

    void bgfxFrameBuffer::Bind() {
        bgfx::setViewFrameBuffer(m_ViewId, m_FrameBufferHandle);
    }

    void bgfxFrameBuffer::Resize(const uint32_t width, const uint32_t height) {
        if (width == 0 || height == 0 || (width == m_Width && height == m_Height)) {
            return;
        }

        m_Width = width;
        m_Height = height;

        if (bgfx::isValid(m_FrameBufferHandle)) {
            bgfx::destroy(m_FrameBufferHandle);
        }

        for (const auto& attachment : m_Attachments) {
            attachment->Resize(width, height);
        }

        m_FrameBufferHandle = CreateFrameBufferHandle(m_Attachments);
    }
}
