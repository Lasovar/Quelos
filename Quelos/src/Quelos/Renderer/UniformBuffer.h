#pragma once

#include "Quelos/Utility/SlotMap.h"

namespace Quelos {
    enum class UniformBufferType {
        Sampler, //!< Sampler.
        Float4,    //!< 4 floats vector.
        Mat3,    //!< 3x3 matrix.
        Mat4,    //!< 4x4 matrix.
        Count
    };

    class UniformBuffer;

    struct UniformBufferHandle : Handle<UniformBuffer> {
        UniformBufferHandle() = default;
        UniformBufferHandle(const Handle handle) : Handle(handle) {}
    };
}
