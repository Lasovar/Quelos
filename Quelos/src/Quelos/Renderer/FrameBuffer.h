#pragma once

#include "Quelos/Renderer/Texture.h"

namespace Quelos {
    class FrameBuffer : public RefCounted {
    public:
        virtual void Bind(uint32_t viewId) = 0;

        virtual uint32_t GetWidth() const = 0;
        virtual uint32_t GetHeight() const = 0;

        virtual uint32_t GetViewID() const = 0;

        virtual void Resize(uint32_t width, uint32_t height) = 0;
    public:
        static Ref<FrameBuffer> CreateFrameBuffer(const std::vector<Ref<Texture2D>>& attachments);
    };
}
