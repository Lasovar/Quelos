#pragma once

namespace Quelos {
    template <typename T>
    concept TupleLike = requires(T t) {
        t[0] && T::length();
    };
}