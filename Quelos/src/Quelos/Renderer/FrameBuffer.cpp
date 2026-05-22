#include "qspch.h"
#include "FrameBuffer.h"

#include "Renderer.h"

namespace Quelos {
    FrameBuffer::~FrameBuffer() {
        Renderer::Destroy(m_Handle);
    }

    uint32_t FrameBuffer::GetWidth() const {
        return Renderer::FrameBufferGetWidth(m_Handle);
    }

    uint32_t FrameBuffer::GetHeight() const {
        return Renderer::FrameBufferGetHeight(m_Handle);
    }

    void FrameBuffer::Resize(const uint32_t width, const uint32_t height) const {
        Renderer::FrameBufferResize(m_Handle, width, height);
    }

    Ref<FrameBuffer> FrameBuffer::Create(const Span<Ref<Texture2D>>& attachments, RenderPassHandle renderPassHandle) {
        SmallVec<TextureHandle, 4> textureHandles;
        for (const auto& attachment : attachments) {
            textureHandles.push_back(attachment->GetHandle());
        }

        return CreateRef<FrameBuffer>(Renderer::CreateFrameBuffer(TODO
        ));
    }
}
