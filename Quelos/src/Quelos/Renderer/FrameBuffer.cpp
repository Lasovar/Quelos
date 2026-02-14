#include "qspch.h"
#include "FrameBuffer.h"

#include "Quelos/Platform/bgfx/bgfxFrameBuffer.h"

namespace Quelos {
    Ref<FrameBuffer> FrameBuffer::CreateFrameBuffer(uint32_t viewID, const std::vector<Ref<Texture2D>>& attachments) {
        return CreateRef<bgfxFrameBuffer>(viewID, attachments);
    }
}
