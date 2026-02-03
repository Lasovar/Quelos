#include "qspch.h"
#include "FrameBuffer.h"

#include "Quelos/Platform/bgfx/bgfxFrameBuffer.h"

namespace Quelos {
    Ref<FrameBuffer> FrameBuffer::CreateFrameBuffer(const std::vector<Ref<Texture2D>>& attachments) {
        return CreateRef<bgfxFrameBuffer>(attachments);
    }
}
