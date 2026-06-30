#pragma once

#include "Extent2D.h"
#include "RenderPass.h"
#include "Quelos/Renderer/Texture.h"

namespace Quelos {
    // Holds some temp values... keep temp values alive until creation
    struct FrameBufferSpec {
        std::string_view Name;
        Span32<const TextureViewHandle> Attachments;
        RenderPassHandle RenderPassHandle;
        uint32_t NumArraySlices = 0;
        Extent2D Size;
    };

    class FrameBuffer;

    struct QS_API FrameBufferHandle : Handle<FrameBuffer> {
        FrameBufferHandle() = default;
        FrameBufferHandle(const Handle handle) : Handle(handle) {}
    };

    class QS_API FrameBuffer {
    public:
        FrameBuffer(const FrameBufferHandle handle) : m_Handle(handle) {}
        ~FrameBuffer();

        [[nodiscard]] uint32_t GetWidth() const;
        [[nodiscard]] uint32_t GetHeight() const;

        void Resize(uint32_t width, uint32_t height) const;
        [[nodiscard]] FrameBufferHandle GetHandle() const { return m_Handle; }

    public:
        static Ref<FrameBuffer> Create(const FrameBufferSpec& frameBufferSpec);
    private:
        FrameBufferHandle m_Handle;
    };
}
