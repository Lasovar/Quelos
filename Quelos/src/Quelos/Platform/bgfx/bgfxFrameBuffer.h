#pragma once

#include "Quelos/Renderer/FrameBuffer.h"

#include "bgfx/bgfx.h"

namespace Quelos {
    class bgfxFrameBuffer : public FrameBuffer {
    public:
        explicit bgfxFrameBuffer(const std::vector<Ref<Texture2D>>& attachments);

        void Bind(uint32_t viewId) override;

        uint32_t GetWidth() const override { return m_Width; }
        uint32_t GetHeight() const override { return m_Height; }

        uint32_t GetViewID() const override { return m_ViewId; }

        void Resize(uint32_t width, uint32_t height) override;

    private:
        uint32_t m_Width = 1;
        uint32_t m_Height = 1;

        bgfx::ViewId m_ViewId = BGFX_INVALID_HANDLE;

        std::vector<Ref<Texture2D>> m_Attachments;
        bgfx::FrameBufferHandle m_FrameBufferHandle = BGFX_INVALID_HANDLE;
    };
}
