#include <qspch.h>
#include "IndexBuffer.h"

#include "Quelos/Platform/bgfx/bgfxIndexBuffer.h"

namespace Quelos {

    Ref<IndexBuffer> IndexBuffer::Create(const std::vector<uint16_t>& indices) {
        return CreateRef<bgfxIndexBuffer>(indices);
    }
}
