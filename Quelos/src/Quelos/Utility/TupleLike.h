#pragma once

#include "Quelos/Math/Math.h"

namespace Quelos {
    template <typename T>
    concept TupleLike = requires(T t) {
        t[0] && math::count<T>;
    };

    template <typename T>
    concept FloatType = requires(T t) {
        t.f32 && math::count<T>;
    };

    template <typename T>
    concept DoubleType = requires(T t) {
        t.d32 && math::count<T>;
    };
}
