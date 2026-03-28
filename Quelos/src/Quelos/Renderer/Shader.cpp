#include "Shader.h"
#include "Renderer.h"

namespace Quelos {

    void ShaderHandle::Submit(const uint32_t viewId) const {
        Renderer::Submit(*this, viewId);
    }
}
