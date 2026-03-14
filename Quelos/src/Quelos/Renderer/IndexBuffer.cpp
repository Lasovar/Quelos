#include "IndexBuffer.h"

#include "Renderer.h"

namespace Quelos {
    void IndexBufferHandle::Bind() const {
        Renderer::BindIndexBuffer(*this);
    }
}
