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

        auto operator<=>(const ShaderResourceBindingHandle&) const = default;
    };
}

template<>
struct std::hash<Quelos::ShaderResourceBindingHandle> {
    std::size_t operator()(const Quelos::ShaderResourceBindingHandle& handle) const noexcept {
        return handle.Value;
    }
};
