//
// Created by lasovar on 5/24/26.
//

#pragma once
#include "Quelos/Utility/SlotMap.h"

namespace Quelos {
    class ShaderResourceBinding;

    struct ShaderResourceBindingHandle : Handle<ShaderResourceBinding> {
        ShaderResourceBindingHandle() = default;
        ShaderResourceBindingHandle(Handle handle) : Handle(handle) {}
    };
}
