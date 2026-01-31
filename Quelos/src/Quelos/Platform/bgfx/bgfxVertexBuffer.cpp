#include "qspch.h"
#include "bgfxVertexBuffer.h"

namespace Quelos {

    bgfxVertexBuffer::bgfxVertexBuffer(const std::vector<PosColorVertex>& vertices) {
        bgfx::VertexLayout pvcLayout;
        pvcLayout
            .begin()
            .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
            .end();

        m_Handle = createVertexBuffer(bgfx::makeRef(vertices.data(), vertices.size() * sizeof(PosColorVertex)), pvcLayout);

        if (!bgfx::isValid(m_Handle)) {
            QS_CORE_ERROR("Failed to create VertexBuffer!");
        }
    }

    void bgfxVertexBuffer::Bind(const uint32_t stream) const {
        bgfx::setVertexBuffer(stream, m_Handle);
    }

    bgfxVertexBuffer::~bgfxVertexBuffer() {
        bgfx::destroy(m_Handle);
    }
}
