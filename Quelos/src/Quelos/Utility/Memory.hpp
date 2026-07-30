//
// Created by lasovar on 7/12/26.
//

#pragma once

#include <cstring>
#include <memory>
#include <type_traits>

namespace Quelos::memory {
    template <class T, std::integral SizeType>
        requires (std::is_unsigned_v<SizeType>)
    constexpr std::size_t byte_count(SizeType n) noexcept {
        return static_cast<std::size_t>(n) * sizeof(T);
    }

    constexpr void* copy(void* destination, const void* source, const std::size_t count) {
        return std::memcpy(destination, source, count);
    }

    constexpr void* move(void* destination, const void* source, const std::size_t count) {
        return std::memmove(destination, source, count);
    }

    constexpr void* set(void* destination, const int value, const std::size_t count) {
        return std::memset(destination, value, count);
    }

    using std::construct_at;
    using std::destroy_at;

    template <typename T>
    void destroy_range(T* first, T* last) noexcept {
        if constexpr (!std::is_trivially_destructible_v<T>) {
            std::destroy(first, last);
        }
    }

    template <typename T, typename SizeType>
        requires (std::is_integral_v<SizeType> && std::is_unsigned_v<SizeType>)
    void destroy_range(T* data, const SizeType count) {
        if constexpr (!std::is_trivially_destructible_v<T>) {
            std::destroy_n(data, count);
        }
    }

    // Moves (or memcpy's) `count` elements from `src` into brand-new, non-overlapping
    // storage at `dst`, and ends the lifetime of the source elements. Used whenever
    // the source buffer is about to be freed wholesale (reallocation paths).
    template <typename T, typename SizeType>
        requires (std::is_unsigned_v<SizeType> && std::is_integral_v<SizeType>)
    void relocate_range(T* dst, T* src, SizeType count) {
        if constexpr (std::is_trivially_copyable_v<T>) {
            memory::copy(dst, src, byte_count<T>(count));
        }
        else {
            std::uninitialized_move_n(src, static_cast<std::size_t>(count), dst);
            std::destroy_n(src, count);
        }
    }

    // Moves (or copy's) `count` elements from `src` into brand-new, non-overlapping
    // storage at `dst`, and ends the lifetime of the source elements. Used whenever
    // the source buffer is about to be freed wholesale (reallocation paths).
    // Uses allocator-aware construction when T participates in uses_allocator construction.
    template <typename T, typename SizeType, typename Allocator>
        requires (std::is_unsigned_v<SizeType> && std::is_integral_v<SizeType>)
    void relocate_range(T* dst, T* src, SizeType count, const Allocator& allocator) {
        if constexpr (std::is_trivially_copyable_v<T>) {
            if (count > 0) {
                memory::copy(dst, src, byte_count<T>(count));
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

            destroy_range(src, src + static_cast<std::size_t>(count));
        }
    }

    template <typename T, typename SizeType>
        requires (std::is_integral_v<SizeType> && std::is_unsigned_v<SizeType>)
    void uninitialized_copy_range(T* dst, const T* src, const SizeType count) {
        if constexpr (std::is_trivially_copyable_v<T>) {
            memory::copy(dst, src, byte_count<T>(count));
        }
        else {
            std::uninitialized_copy_n(src, count, dst);
        }
    }

    template <typename T, typename SizeType, typename Allocator>
        requires (std::is_integral_v<SizeType> && std::is_unsigned_v<SizeType>)
    void uninitialized_copy_range(T* dst, const T* src, const SizeType count, const Allocator& allocator) {
        if constexpr (std::is_trivially_copyable_v<T>) {
            memory::copy(dst, src, byte_count<T>(count));
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
