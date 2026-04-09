#pragma once

#include "Quelos/Utility/SlotMap.h"

namespace Quelos {
    class QS_API Shader;
    struct QS_API ShaderHandle : Handle<Shader> {
        ShaderHandle() = default;
        ShaderHandle(const Handle shaderHandle) {
            Value = shaderHandle.Value;
        }

        void Submit(uint32_t viewId) const;
    };
}
