#include "qspch.h"
#include "bgfxIndexBuffer.h"

namespace Quelos {
    bgfxIndexBuffer::bgfxIndexBuffer(const std::vector<uint16_t>& indices) {
        m_Handle = bgfx::createIndexBuffer(bgfx::makeRef(indices.data(), indices.size() * sizeof(uint16_t)));

        if (!bgfx::isValid(m_Handle)) {
            QS_CORE_ERROR("Failed to create IndexBuffer!");
        }
    }
    
    void bgfxIndexBuffer::Bind() const { bgfx::setIndexBuffer(m_Handle); }

    bgfxIndexBuffer::~bgfxIndexBuffer() { bgfx::destroy(m_Handle); }
}
