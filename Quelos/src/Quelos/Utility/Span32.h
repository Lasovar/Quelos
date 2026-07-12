//
// Created by lasovar on 5/26/26.
//

#pragma once

#include <assert.h>
#include <cstdint>
#include <vector>
#include <cassert>
#include <span>
#include <array>
#include <type_traits>

namespace Quelos {
    template <typename T>
    class Span32 {
    public:
        using element_type = T;
        using value_type = std::remove_cv_t<T>;
        using pointer = T*;
        using reference = T&;
        using iterator = T*;
        using size_type = uint32_t;

    public:
        constexpr Span32() noexcept = default;

        constexpr Span32(std::nullptr_t) noexcept
            : m_Data(nullptr) {}

        constexpr Span32(const pointer data, const uint32_t size) noexcept
            : m_Data(data), m_Size(size) {}

        constexpr Span32(pointer begin, pointer end) noexcept
            : m_Data(begin), m_Size(static_cast<uint32_t>(end - begin)
        ) {
            assert(end >= begin);
        }

        template <size_t N>
        constexpr Span32(T (&arr)[N]) noexcept
            : m_Data(arr), m_Size(static_cast<uint32_t>(N)) {}

        template <typename U>
            requires std::is_convertible_v<U (*)[], T (*)[]>
        constexpr Span32(const Span32<U>& other) noexcept
            : m_Data(other.data()), m_Size(other.size()) {}

        template <typename U>
            requires std::is_convertible_v<U (*)[], T (*)[]>
        constexpr Span32(std::span<U> other) noexcept
            : m_Data(other.data()), m_Size(static_cast<uint32_t>(other.size())
              ) {
            assert(other.size() <= UINT32_MAX);
        }

        template <typename U, typename Alloc>
            requires std::is_convertible_v<U (*)[], T (*)[]>
        constexpr Span32(std::vector<U, Alloc>& vec) noexcept
            : m_Data(vec.data()), m_Size(static_cast<uint32_t>(vec.size())) {
            assert(vec.size() <= UINT32_MAX);
        }

        template <typename U, typename SizeType>
            requires std::is_convertible_v<U (*)[], T (*)[]>
        constexpr Span32(Vec<U, SizeType>& vec) noexcept
            : m_Data(vec.data()), m_Size(static_cast<uint32_t>(vec.size())) {
            assert(vec.size() <= UINT32_MAX);
        }

        template <typename U, uint32_t N>
            requires std::is_convertible_v<U (*)[], T (*)[]>
        constexpr Span32(SmallVec<U, N>& vec)
            : m_Data(vec.data()), m_Size(vec.size()) {}

        template <typename U, uint32_t N>
            requires std::is_convertible_v<const U (*)[], T (*)[]>
        constexpr Span32(const SmallVec<U, N>& vec)
            : m_Data(vec.data()), m_Size(vec.size()) {}

        template <typename U, typename Alloc>
            requires std::is_convertible_v<const U (*)[], T (*)[]>
        constexpr Span32(const std::vector<U, Alloc>& vec) noexcept
            : m_Data(vec.data()), m_Size(static_cast<uint32_t>(vec.size())) {
            assert(vec.size() <= UINT32_MAX);
        }

        template <typename U, size_t N>
            requires std::is_convertible_v<U (*)[], T (*)[]>
        constexpr Span32(std::array<U, N>& arr) noexcept
            : m_Data(arr.data()), m_Size(static_cast<uint32_t>(N)) {}

        template <typename U, size_t N>
            requires std::is_convertible_v<const U (*)[], T (*)[]>
        constexpr Span32(const std::array<U, N>& arr) noexcept
            : m_Data(arr.data()), m_Size(static_cast<uint32_t>(N)) {}

    public:
        [[nodiscard]] constexpr pointer data() const noexcept {
            return m_Data;
        }

        [[nodiscard]] constexpr uint32_t size() const noexcept {
            return m_Size;
        }

        [[nodiscard]] constexpr size_t size_bytes() const noexcept {
            return m_Size * sizeof(T);
        }

        [[nodiscard]] constexpr bool empty() const noexcept {
            return m_Size == 0;
        }

        [[nodiscard]] constexpr reference operator[](uint32_t index) const noexcept {
            assert(index < m_Size);
            return m_Data[index];
        }

        [[nodiscard]] constexpr reference front() const noexcept {
            assert(m_Size > 0);
            return m_Data[0];
        }

        [[nodiscard]]
        constexpr reference back() const noexcept {
            assert(m_Size > 0);
            return m_Data[m_Size - 1];
        }

        [[nodiscard]] constexpr iterator begin() const noexcept {
            return m_Data;
        }

        [[nodiscard]] constexpr iterator end() const noexcept {
            return m_Data + m_Size;
        }

        [[nodiscard]] constexpr Span32 first(uint32_t count) const noexcept {
            assert(count <= m_Size);
            return Span32(m_Data, count);
        }

        [[nodiscard]] constexpr Span32 last(uint32_t count) const noexcept {
            assert(count <= m_Size);
            return Span32(m_Data + (m_Size - count), count);
        }

        [[nodiscard]] constexpr Span32 subspan(uint32_t offset, uint32_t count) const noexcept {
            assert(offset <= m_Size);
            assert(offset + count <= m_Size);

            return Span32(m_Data + offset, count);
        }

        [[nodiscard]] constexpr explicit operator std::span<T>() const noexcept {
            return std::span<T>(m_Data, m_Size);
        }

    private:
        pointer m_Data = nullptr;
        uint32_t m_Size = 0;
    };

    template <typename T, uint32_t N>
    Span32(SmallVec<T, N>&) -> Span32<T>;

    template <typename T, uint32_t N>
    Span32(const SmallVec<T, N>&) -> Span32<const T>;

    template <typename T, size_t N>
    Span32(std::array<T, N>&) -> Span32<T>;

    template <typename T, size_t N>
    Span32(const std::array<T, N>&) -> Span32<const T>;
}
