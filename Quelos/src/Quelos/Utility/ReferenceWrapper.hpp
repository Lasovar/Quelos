//
// Created by lasovar on 7/26/26.
//

#pragma once

#include <functional>

namespace Quelos {
    template <typename T>
    using Ref = std::reference_wrapper<T>;

    template <typename T>
    Ref<T> GetRef(T& t) {
        return std::ref(t);
    }

    template <typename T>
    Ref<const T> GetRef(const T& t) {
        return std::cref(t);
    }
}
