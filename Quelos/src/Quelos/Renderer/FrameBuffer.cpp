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

    SharedPtr<FrameBuffer> FrameBuffer::Create(const FrameBufferSpec& frameBufferSpec) {
        return CreateShared<FrameBuffer>(Renderer::CreateFrameBuffer(frameBufferSpec));
    }
}
