#pragma once

#include "Quelos/Utility/SlotMap.h"

namespace Quelos {
    class RenderPass;

    struct RenderPassHandle : Handle<RenderPass> {
        RenderPassHandle() = default;
        RenderPassHandle(const Handle handle) : Handle(handle) {}
    };
}
