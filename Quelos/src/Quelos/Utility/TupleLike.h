#pragma once

#include "Quelos/Math/Math.h"

namespace Quelos {
    template <typename T>
    concept TupleLike = requires(T t) {
        t[0];
        math::count<T>();
    };

    template <typename T>
    concept FloatType = requires(T t) {
        t.f32;
        math::count<T>();
    };

    template <typename T>
    concept DoubleType = requires(T t) {
        t.f64;
        math::count<T>();
    };

    template <typename T>
    concept VectorType = (FloatType<T> || DoubleType<T>) && requires(T t) {
        math::value_ptr(t);
    };
}
