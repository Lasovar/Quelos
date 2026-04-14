#pragma once

#include "Quelos/Renderer/Texture.h"

namespace Quelos {
    class FrameBuffer;

    struct QS_API FrameBufferHandle : Handle<FrameBuffer> {
        FrameBufferHandle() = default;
        FrameBufferHandle(const Handle handle) : Handle(handle) {}
    };

    class QS_API FrameBuffer {
    public:
        FrameBuffer(const FrameBufferHandle handle) : m_Handle(handle) {}
        ~FrameBuffer() = default;

        void Bind() const;

        [[nodiscard]] uint32_t GetWidth() const;
        [[nodiscard]] uint32_t GetHeight() const;

        void SetViewID(uint32_t viewID) const;
        [[nodiscard]] uint32_t GetViewID() const;

        void Resize(uint32_t width, uint32_t height) const;
        [[nodiscard]] FrameBufferHandle GetHandle() const { return m_Handle; }

    public:
        static Ref<FrameBuffer> Create(uint32_t viewID, const Span<Ref<Texture2D>>& attachments);
    private:
        FrameBufferHandle m_Handle;
    };
}
