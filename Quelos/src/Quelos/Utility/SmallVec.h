#pragma once

#include <cstddef>
#include <cstring>
#include <new>
#include <utility>
#include <type_traits>

namespace Quelos {
    template<typename T, size_t N>
    class SmallVec {
    public:
        using value_type = T;
        using size_type = size_t;
        using iterator = T*;
        using const_iterator = const T*;

    public:
        SmallVec() noexcept
            : m_Data(inline_ptr()), m_Size(0), m_Capacity(N) {}

        ~SmallVec() {
            clear();
            if (!is_inline()) {
                operator delete(m_Data);
            }
        }

        SmallVec(const SmallVec& other) {
            init_from(other);
        }

        SmallVec(SmallVec&& other) noexcept {
            move_from(std::move(other));
        }

        SmallVec& operator=(const SmallVec& other) {
            if (this != &other) {
                clear();
                if (!is_inline()) {
                    operator delete(m_Data);
                }

                init_from(other);
            }

            return *this;
        }

        SmallVec& operator=(SmallVec&& other) noexcept {
            if (this != &other) {
                clear();
                if (!is_inline()) {
                    operator delete(m_Data);
                }

                move_from(std::move(other));
            }

            return *this;
        }

        T& operator[](size_t i) { return m_Data[i]; }
        const T& operator[](size_t i) const { return m_Data[i]; }

        T* data() { return m_Data; }
        const T* data() const { return m_Data; }

        iterator begin() { return m_Data; }
        iterator end() { return m_Data + m_Size; }
        const_iterator begin() const { return m_Data; }
        const_iterator end() const { return m_Data + m_Size; }

        [[nodiscard]] size_t size() const { return m_Size; }
        [[nodiscard]] size_t capacity() const { return m_Capacity; }
        [[nodiscard]] bool empty() const { return m_Size == 0; }

        void reserve(const size_t newCap) {
            if (newCap <= m_Capacity) {
                return;
            }

            grow_to(newCap);
        }

        void clear() {
            destroy_range(m_Data, m_Size);
            m_Size = 0;
        }

        template<typename... Args>
        T& emplace_back(Args&&... args) {
            if (m_Size >= m_Capacity) {
                grow();
            }

            T* ptr = m_Data + m_Size;
            new (ptr) T(std::forward<Args>(args)...);
            m_Size++;
            return *ptr;
        }

        void push_back(const T& value) {
            emplace_back(value);
        }

        void push_back(T&& value) {
            emplace_back(std::move(value));
        }

        void pop_back() {
            m_Data[--m_Size].~T();
        }

    private:
        T* inline_ptr() {
            return reinterpret_cast<T*>(m_Inline);
        }

        const T* inline_ptr() const {
            return reinterpret_cast<const T*>(m_Inline);
        }

        [[nodiscard]] bool is_inline() const {
            return m_Data == inline_ptr();
        }

        void grow() {
            grow_to(m_Capacity * 2);
        }

        void grow_to(const size_t newCap) {
            T* newData = static_cast<T*>(operator new(newCap * sizeof(T)));

            move_range(newData, m_Data, m_Size);
            destroy_range(m_Data, m_Size);

            if (!is_inline()) {
                operator delete(m_Data);
            }

            m_Data = newData;
            m_Capacity = newCap;
        }

        void init_from(const SmallVec& other) {
            if (other.m_Size <= N) {
                m_Data = inline_ptr();
                m_Capacity = N;
            } else {
                m_Data = static_cast<T*>(operator new(other.m_Size * sizeof(T)));
                m_Capacity = other.m_Size;
            }

            copy_range(m_Data, other.m_Data, other.m_Size);
            m_Size = other.m_Size;
        }

        void move_from(SmallVec&& other) {
            if (other.is_inline()) {
                m_Data = inline_ptr();
                m_Capacity = N;
                move_range(m_Data, other.m_Data, other.m_Size);
            } else {
                m_Data = other.m_Data;
                m_Capacity = other.m_Capacity;
                other.m_Data = other.inline_ptr();
                other.m_Capacity = N;
            }

            m_Size = other.m_Size;
            other.m_Size = 0;
        }

        static void destroy_range(T* data, const size_t count) {
            if constexpr (!std::is_trivially_destructible_v<T>) {
                for (size_t i = 0; i < count; i++) {
                    data[i].~T();
                }
            }
        }

        static void move_range(T* dst, T* src, const size_t count) {
            if constexpr (std::is_trivially_copyable_v<T>) {
                std::memcpy(dst, src, count * sizeof(T));
            } else {
                for (size_t i = 0; i < count; ++i) {
                    new (&dst[i]) T(std::move(src[i]));
                }
            }
        }

        static void copy_range(T* dst, const T* src, const size_t count) {
            if constexpr (std::is_trivially_copyable_v<T>) {
                std::memcpy(dst, src, count * sizeof(T));
            } else {
                for (size_t i = 0; i < count; ++i) {
                    new (&dst[i]) T(src[i]);
                }
            }
        }

    private:
        alignas(T) unsigned char m_Inline[sizeof(T) * N]{};

        T* m_Data;
        size_t m_Size;
        size_t m_Capacity;
    };
}
