//
// Created by luais on 07/07/2026.
//

#pragma once

#include "Quelos/Utility/SlotMap.h"

namespace Quelos {
    enum class FenceType : uint8_t {
        /// Basic fence that may be used for:
        ///  - signaling the fence from GPU
        ///  - waiting for the fence on CPU
        CpuWaitOnly = 0,

        /// General fence that may be used for:
        ///  - signaling the fence from GPU
        ///  - waiting for the fence on CPU
        ///  - waiting for the fence on GPU
        ///
        /// If NativeFence feature is enabled (see Diligent::DeviceFeatures), the fence may also be used for:
        ///  - signaling the fence on CPU
        ///  - waiting on GPU for a value that will be enqueued for signal later
        General = 1,

        Last = General
    };

    struct FenceSpec {
        std::string_view Name;
        FenceType Type = FenceType::CpuWaitOnly;
    };

    class Fence;

    struct FenceHandle : Handle<Fence> {
        FenceHandle() = default;
        FenceHandle(const Handle handle) : Handle(handle) {}
    };
}
