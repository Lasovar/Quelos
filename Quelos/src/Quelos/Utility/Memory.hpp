//
// Created by lasovar on 7/12/26.
//

#pragma once

#include <cstring>
#include <memory>
#include <type_traits>

namespace Quelos::memory {
    using std::memcpy;
    using std::memmove;
    using std::memset;
    using std::construct_at;
    using std::destroy_at;

    template <typename T>
    void destroy_range(T* first, T* last) noexcept {
        if constexpr (!std::is_trivially_destructible_v<T>) {
            for (T* p = first; p != last; ++p) {
                destroy_at(p);
            }
        }
    }

    // Moves (or memcpy's) `count` elements from `src` into brand-new, non-overlapping
    // storage at `dst`, and ends the lifetime of the source elements. Used whenever
    // the source buffer is about to be freed wholesale (reallocation paths).
    template <typename T, typename SizeType>
        requires (std::is_unsigned_v<SizeType> && std::is_integral_v<SizeType>)
    static void relocate_range(T* dst, T* src, SizeType count) {
        if constexpr (std::is_trivially_copyable_v<T>) {
            if (count > 0) {
                memcpy(dst, src, static_cast<std::size_t>(count) * sizeof(T));
            }
        }
        else {
            for (SizeType i = 0; i < count; ++i) {
                construct_at(dst + i, std::move_if_noexcept(src[i]));
            }

            destroy_range(src, src + count);
        }
    }
}
