#include "VertexBuffer.h"

#include "Renderer.h"

namespace Quelos {
    void VertexBufferHandle::Bind(const uint32_t stream) const {
        Renderer::BindVertexBuffer(*this, stream);
    }
}

