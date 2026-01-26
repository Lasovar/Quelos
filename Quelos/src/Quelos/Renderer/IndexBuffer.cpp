#include <qspch.h>
#include "IndexBuffer.h"

namespace Quelos {
    IndexBuffer::IndexBuffer(const std::vector<uint16_t>& indices) {
        m_Handle = bgfx::createIndexBuffer(bgfx::makeRef(indices.data(), indices.size() * sizeof(uint16_t)));

        if (!bgfx::isValid(m_Handle)) {
            QS_CORE_ERROR("Failed to create IndexBuffer!");
        }
    }

    IndexBuffer::~IndexBuffer() {
        bgfx::destroy(m_Handle);
    }
}
