#pragma once

#include "Quelos/Renderer/IndexBuffer.h"

#include <bgfx/bgfx.h>

namespace Quelos {
    class bgfxIndexBuffer : public IndexBuffer {
    public:
        explicit bgfxIndexBuffer(const std::vector<uint16_t>& indices);
        ~bgfxIndexBuffer() override;

        void Bind() const override;
    private:
        bgfx::IndexBufferHandle m_Handle = BGFX_INVALID_HANDLE;
    };
}
