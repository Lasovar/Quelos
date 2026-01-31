#pragma once
#include "Quelos/Renderer/VertexBuffer.h"

#include <bgfx/bgfx.h>

namespace Quelos {
    class bgfxVertexBuffer : public VertexBuffer {
    public:
        explicit bgfxVertexBuffer(const std::vector<PosColorVertex>& vertices);
        ~bgfxVertexBuffer() override;

        void Bind(uint32_t stream) const override;

    private:
		bgfx::VertexBufferHandle m_Handle = BGFX_INVALID_HANDLE;
    };
}
