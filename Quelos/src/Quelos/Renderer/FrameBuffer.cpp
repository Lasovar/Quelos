#include "qspch.h"
#include "FrameBuffer.h"

#include "Renderer.h"

namespace Quelos {
    void FrameBuffer::Bind() const {
        Renderer::Bind(m_Handle);
    }

    uint32_t FrameBuffer::GetWidth() const {
        return Renderer::FrameBufferGetWidth(m_Handle);
    }

    uint32_t FrameBuffer::GetHeight() const {
        return Renderer::FrameBufferGetHeight(m_Handle);
    }

    void FrameBuffer::SetViewID(const uint32_t viewID) const {
        Renderer::FrameBufferSetViewID(m_Handle, viewID);
    }

    uint32_t FrameBuffer::GetViewID() const {
        return Renderer::FrameBufferGetViewID(m_Handle);
    }

    void FrameBuffer::Resize(const uint32_t width, const uint32_t height) const {
        Renderer::FrameBufferResize(m_Handle, width, height);
    }

    Ref<FrameBuffer> FrameBuffer::Create(const uint32_t viewID, const Span<Ref<Texture2D>>& attachments) {
        SmallVec<TextureHandle, 4> textureHandles;
        for (const auto& attachment : attachments) {
            textureHandles.push_back(attachment->GetHandle());
        }

        return CreateRef<FrameBuffer>(Renderer::CreateFrameBuffer(viewID, std::span(textureHandles)));
    }
}
