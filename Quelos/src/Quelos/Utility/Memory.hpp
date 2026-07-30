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

    template <typename T, typename SizeType>
        requires (std::is_integral_v<SizeType> && std::is_unsigned_v<SizeType>)
    void destroy_range(T* data, const SizeType count) {
        if constexpr (!std::is_trivially_destructible_v<T>) {
            for (SizeType i = 0; i < count; ++i) {
                destroy_at(data + count);
            }
        }
    }

    // Moves (or memcpy's) `count` elements from `src` into brand-new, non-overlapping
    // storage at `dst`, and ends the lifetime of the source elements. Used whenever
    // the source buffer is about to be freed wholesale (reallocation paths).
    template <typename T, typename SizeType>
        requires (std::is_unsigned_v<SizeType> && std::is_integral_v<SizeType>)
    static void move_range(T* dst, T* src, SizeType count) {
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

    // Moves (or memcpy's) `count` elements from `src` into brand-new, non-overlapping
    // storage at `dst`, and ends the lifetime of the source elements. Used whenever
    // the source buffer is about to be freed wholesale (reallocation paths).
    template <typename T, typename SizeType, typename Allocator>
        requires (std::is_unsigned_v<SizeType> && std::is_integral_v<SizeType>)
    void move_range(T* dst, T* src, SizeType count, const Allocator& allocator) {
        if constexpr (std::is_trivially_copyable_v<T>) {
            if (count > 0) {
                memcpy(dst, src, static_cast<std::size_t>(count) * sizeof(T));
            }
        }
        else {
            for (SizeType i = 0; i < count; ++i) {
                if constexpr (std::uses_allocator_v<T, Allocator>) {
                    std::uninitialized_construct_using_allocator(
                        dst + i,
                        allocator,
                        std::move_if_noexcept(src[i])
                    );
                }
                else {
                    construct_at(dst + i, std::move_if_noexcept(src[i]));
                }
            }

            destroy_range(src, src + count);
        }
    }

    template <typename T, typename SizeType>
        requires (std::is_integral_v<SizeType> && std::is_unsigned_v<T>)
    void copy_range(T* dst, const T* src, const SizeType count) {
        if constexpr (std::is_trivially_copyable_v<T>) {
            std::memcpy(dst, src, count * sizeof(T));
        }
        else {
            for (SizeType i = 0; i < count; ++i) {
                construct_at(dst + i, src[i]);
            }
        }
    }

    template <typename T, typename SizeType, typename Allocator>
        requires (std::is_integral_v<SizeType> && std::is_unsigned_v<T>)
    void copy_range(T* dst, const T* src, const SizeType count, const Allocator& allocator) {
        if constexpr (std::is_trivially_copyable_v<T>) {
            std::memcpy(dst, src, count * sizeof(T));
        }
        else {
            for (SizeType i = 0; i < count; ++i) {
                if constexpr (std::uses_allocator_v<T, Allocator>) {
                    std::uninitialized_construct_using_allocator(
                        dst + i,
                        allocator,
                        src[i]
                    );
                }
                else {
                    construct_at(dst + i, src[i]);
                }
            }
        }
    }
}
