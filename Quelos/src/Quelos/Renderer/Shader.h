#pragma once

#include "Quelos/Utility/SlotMap.h"

namespace Quelos {
    class Shader;
    struct ShaderHandle : Handle<Shader> {
        ShaderHandle() = default;
        ShaderHandle(const Handle shaderHandle) {
            Value = shaderHandle.Value;
        }

        void Submit(uint32_t viewId) const;
    };
}
