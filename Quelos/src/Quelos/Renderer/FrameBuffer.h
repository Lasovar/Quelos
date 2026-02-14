#pragma once

#include "Quelos/Renderer/Texture.h"

namespace Quelos {
    class FrameBuffer : public RefCounted {
    public:
        virtual void Bind() = 0;

        virtual uint32_t GetWidth() const = 0;
        virtual uint32_t GetHeight() const = 0;

        virtual void SetViewID(uint32_t viewID) = 0;
        virtual uint32_t GetViewID() const = 0;

        virtual void Resize(uint32_t width, uint32_t height) = 0;
    public:
        static Ref<FrameBuffer> CreateFrameBuffer(uint32_t viewID, const std::vector<Ref<Texture2D>>& attachments);
    };
}
