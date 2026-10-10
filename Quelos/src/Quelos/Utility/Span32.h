//
// Created by lasovar on 5/26/26.
//

#pragma once

#include <cstdint>
#include <vector>
#include <cassert>
#include <span>
#include <limits>
#include <concepts>
#include <array>
#include <type_traits>

namespace Quelos {
    template <std::unsigned_integral SizeType>
    inline constexpr SizeType k_DynamicExtent = std::numeric_limits<SizeType>::max();

    inline constexpr uint64_t k_DynamicExtent64 = k_DynamicExtent<uint64_t>;
    inline constexpr uint32_t k_DynamicExtent32 = k_DynamicExtent<uint32_t>;

    namespace Detail {
        template <std::unsigned_integral SizeType, SizeType Extent>
        struct SpanSize {
            using size_type = SizeType;

            constexpr SpanSize() noexcept = default;
            constexpr explicit SpanSize([[maybe_unused]] const size_type size) noexcept {
                assert(size == Extent);
            }

            constexpr operator size_type() const noexcept { return Get(); }
            [[nodiscard]] static constexpr size_type Get() noexcept { return Extent; }
        };

        template <std::unsigned_integral SizeType, SizeType Extent>
            requires (Extent == k_DynamicExtent<SizeType>)
        struct SpanSize<SizeType, Extent> {
            constexpr SpanSize() noexcept = default;
            constexpr explicit SpanSize(const SizeType size) noexcept : m_Size(size) {}

            constexpr operator SizeType() const noexcept { return Get(); }
            [[nodiscard]] constexpr SizeType Get() const noexcept { return m_Size; }
        private:
            SizeType m_Size = 0;
        };

        template <typename From, typename To>
        concept ArrayConvertible = std::is_convertible_v<From (*)[], To (*)[]>;
    }

    template <typename T, std::unsigned_integral SizeType, SizeType Extent = k_DynamicExtent<SizeType>>
    class SpanT {
        static constexpr bool IsDynamic = Extent == k_DynamicExtent<SizeType>;

    public:
        using element_type = T;
        using value_type = std::remove_cv_t<T>;
        using pointer = T*;
        using reference = T&;
        using iterator = T*;
        using size_type = SizeType;

        static constexpr SizeType k_Extent = Extent;

    public:
        constexpr SpanT() noexcept requires (IsDynamic || Extent == 0) = default;

        constexpr explicit(!IsDynamic) SpanT(pointer data, const SizeType size) noexcept
             : m_Data(data), m_Size(size) {}

        template <std::same_as<T*> P>
        constexpr explicit(!IsDynamic) SpanT(P begin, P end) noexcept
            : m_Data(begin), m_Size(static_cast<SizeType>(end - begin)
        ) {
            assert(end >= begin);
        }
        template <Detail::ArrayConvertible<T> U, size_t N>
            requires (IsDynamic || N == Extent)
        constexpr SpanT(U (&arr)[N]) noexcept : m_Data(arr), m_Size(static_cast<SizeType>(N)) {}

        template <Detail::ArrayConvertible<T> U, size_t N>
            requires (IsDynamic || N == Extent)
        constexpr SpanT(std::array<U, N>& arr) noexcept
            : m_Data(arr.data()), m_Size(static_cast<SizeType>(N)) {}

        template <typename U, size_t N>
            requires Detail::ArrayConvertible<const U, T> && (IsDynamic || N == Extent)
        constexpr SpanT(const std::array<U, N>& arr) noexcept
            : m_Data(arr.data()), m_Size(static_cast<SizeType>(N)) {}

        // Other SpanT: static<->static must match, dynamic->static is explicit
        template <Detail::ArrayConvertible<T> U, SizeType E2>
            requires (IsDynamic || E2 == k_DynamicExtent<SizeType> || E2 == Extent)
        constexpr explicit(!IsDynamic && E2 == k_DynamicExtent<SizeType>)
        SpanT(const SpanT<U, SizeType, E2>& other) noexcept
            : m_Data(other.data()), m_Size(other.size()) {}

        template <Detail::ArrayConvertible<T> U, size_t E2>
            requires (IsDynamic || E2 == std::dynamic_extent || E2 == Extent)
        constexpr explicit(!IsDynamic && E2 == std::dynamic_extent)
        SpanT(std::span<U, E2> other) noexcept
            : m_Data(other.data()), m_Size(static_cast<SizeType>(other.size())
        ) {
            assert(other.size() <= std::numeric_limits<SizeType>::max());
        }

        template <Detail::ArrayConvertible<T> U, typename Alloc>
        constexpr explicit(!IsDynamic) SpanT(std::vector<U, Alloc>& vec) noexcept
            : m_Data(vec.data()), m_Size(static_cast<SizeType>(vec.size())
        ) {
            assert(vec.size() <= std::numeric_limits<SizeType>::max());
        }

        template <typename U, typename Alloc>
            requires Detail::ArrayConvertible<const U, T>
        constexpr explicit(!IsDynamic) SpanT(const std::vector<U, Alloc>& vec) noexcept
            : m_Data(vec.data()), m_Size(static_cast<SizeType>(vec.size())
        ) {
            assert(vec.size() <= std::numeric_limits<SizeType>::max());
        }

        template <Detail::ArrayConvertible<T> U, std::unsigned_integral USizeType>
        constexpr explicit(!IsDynamic) SpanT(VectorT<U, USizeType>& vec) noexcept
            : m_Data(vec.data()), m_Size(static_cast<SizeType>(vec.size())
        ) {
            assert(vec.size() <= std::numeric_limits<SizeType>::max());
        }

        template <typename U, std::unsigned_integral USizeType>
            requires Detail::ArrayConvertible<const U, T>
        constexpr explicit(!IsDynamic) SpanT(const VectorT<U, USizeType>& vec) noexcept
            : m_Data(vec.data()), m_Size(static_cast<SizeType>(vec.size())
        ) {
            assert(vec.size() <= std::numeric_limits<SizeType>::max());
        }

        template <Detail::ArrayConvertible<T> U, uint32_t N>
        constexpr explicit(!IsDynamic) SpanT(SmallVec<U, N>& vec)
            : m_Data(vec.data()), m_Size(static_cast<SizeType>(vec.size())) {}

        template <typename U, uint32_t N>
            requires Detail::ArrayConvertible<const U, T>
        constexpr explicit(!IsDynamic) SpanT(const SmallVec<U, N>& vec)
            : m_Data(vec.data()), m_Size(static_cast<SizeType>(vec.size())) {}

    public:
        [[nodiscard]] constexpr pointer data() const noexcept {
            return m_Data;
        }

        [[nodiscard]] constexpr size_type size() const noexcept {
            return m_Size;
        }

        [[nodiscard]] constexpr size_t size_bytes() const noexcept {
            return m_Size * sizeof(T);
        }

        [[nodiscard]] constexpr bool empty() const noexcept {
            return m_Size == 0;
        }

        [[nodiscard]] constexpr reference operator[](size_type index) const noexcept {
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

        template <SizeType Count>
        [[nodiscard]] constexpr SpanT<T, SizeType, Count> first() const noexcept {
            if constexpr (!IsDynamic) {
                static_assert(Count <= Extent);
            }

            assert(Count <= size());
            return SpanT<T, SizeType, Count>(m_Data, Count);
        }

        template <SizeType Count>
        [[nodiscard]] constexpr SpanT<T, SizeType, Count> last() const noexcept {
            if constexpr (!IsDynamic) {
                static_assert(Count <= Extent);
            }

            assert(Count <= size());
            return SpanT<T, SizeType, Count>(m_Data + (size() - Count), Count);
        }

        template <SizeType Offset, SizeType Count = k_DynamicExtent<SizeType>>
        [[nodiscard]] constexpr auto subspan() const noexcept {
            if constexpr (!IsDynamic) {
                static_assert(Offset <= Extent);
                static_assert(Count == k_DynamicExtent<SizeType> || Count <= Extent - Offset);
            }

            assert(Offset <= size());
            assert(Count == k_DynamicExtent<SizeType> || Count <= size() - Offset);

            constexpr SizeType ResultExtent = Count != k_DynamicExtent<SizeType>
                                                  ? Count
                                                  : IsDynamic
                                                      ? k_DynamicExtent<SizeType>
                                                      : Extent - Offset;

            const SizeType n = Count != k_DynamicExtent<SizeType> ? Count : size() - Offset;
            return SpanT<T, SizeType, ResultExtent>(m_Data + Offset, n);
        }

        [[nodiscard]] constexpr SpanT<T, SizeType> first(SizeType count) const noexcept {
            assert(count <= m_Size);
            return SpanT<T, SizeType>(m_Data, count);
        }

        [[nodiscard]] constexpr SpanT<T, SizeType> last(SizeType count) const noexcept {
            assert(count <= m_Size);
            return SpanT<T, SizeType>(m_Data + (m_Size - count), count);
        }

        [[nodiscard]] constexpr SpanT<T, SizeType> subspan(SizeType offset, SizeType count = k_DynamicExtent<SizeType>) const noexcept {
            assert(offset <= size());
            if (count == k_DynamicExtent<SizeType>) {
                count = size() - offset;
            }

            assert(count <= size() - offset);
            return SpanT<T, SizeType>(m_Data + offset, count);
        }

        using StdSpan = std::span<T, IsDynamic ? std::dynamic_extent : static_cast<size_t>(Extent)>;
        [[nodiscard]] constexpr explicit operator StdSpan() const noexcept {
            return StdSpan(m_Data, m_Size);
        }

    private:
        pointer m_Data = nullptr;
        [[no_unique_address]] Detail::SpanSize<SizeType, Extent> m_Size;
    };

    template <typename T, std::unsigned_integral S, S E>
    [[nodiscard]] auto AsBufferView(SpanT<T, S, E> s) noexcept {
        constexpr S ResultExtent = E == k_DynamicExtent<S> ? E : static_cast<S>(E * sizeof(T));
        return SpanT<const std::byte, S, ResultExtent>(
            reinterpret_cast<const std::byte*>(s.data()),
            static_cast<S>(s.size_bytes()));
    }

    template <typename T, std::unsigned_integral S, S E>
        requires (!std::is_const_v<T>)
    [[nodiscard]] auto AsMutBufferView(SpanT<T, S, E> s) noexcept {
        constexpr S ResultExtent = E == k_DynamicExtent<S> ? E : static_cast<S>(E * sizeof(T));
        return SpanT<std::byte, S, ResultExtent>(
            reinterpret_cast<std::byte*>(s.data()),
            static_cast<S>(s.size_bytes()));
    }

    template <typename T, std::integral I, std::unsigned_integral S = size_t>
    SpanT(T*, I) -> SpanT<T, S>;

    template <typename T, std::unsigned_integral S = size_t>
    SpanT(T*, T*) -> SpanT<T, S>;

    template <typename T, size_t N, std::unsigned_integral S = size_t>
    SpanT(T (&)[N]) -> SpanT<T, S, static_cast<S>(N)>;

    template <typename T, size_t N, std::unsigned_integral S = size_t>
    SpanT(std::array<T, N>&) -> SpanT<T, S, static_cast<S>(N)>;

    template <typename T, size_t N, std::unsigned_integral S = size_t>
    SpanT(const std::array<T, N>&) -> SpanT<const T, S, static_cast<S>(N)>;

    template <typename T, typename A, std::unsigned_integral S = typename std::vector<T, A>::size_type>
    SpanT(std::vector<T, A>&) -> SpanT<T, S>;

    template <typename T, typename A, std::unsigned_integral S = typename std::vector<T, A>::size_type>
    SpanT(const std::vector<T, A>&) -> SpanT<const T, S>;

    template <typename T, uint32_t N, std::unsigned_integral S = uint32_t>
    SpanT(SmallVec<T, N>&) -> SpanT<T, S>;

    template <typename T, uint32_t N, std::unsigned_integral S = uint32_t>
    SpanT(const SmallVec<T, N>&) -> SpanT<const T, S>;

    template <typename T, typename VS, std::unsigned_integral S = VS>
    SpanT(VectorT<T, VS>&) -> SpanT<T, S>;

    template <typename T, typename VS, std::unsigned_integral S = VS>
    SpanT(const VectorT<T, VS>&) -> SpanT<const T, S>;
}

// Compatibility
namespace std {
    template <typename T, std::unsigned_integral S, S E>
    [[nodiscard]] auto as_bytes(Quelos::SpanT<T, S, E> s) noexcept {
        return Quelos::AsBufferView(s);
    }
}
