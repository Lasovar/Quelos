#pragma once

#include <cstddef>
#include <cstring>
#include <new>
#include <utility>
#include <type_traits>
#include <cstdint>

namespace Quelos {
    template <typename T, uint32_t N>
    class SmallVec {
    public:
        using value_type = T;
        using size_type = uint32_t;
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

        explicit SmallVec(std::span<const T> src) {
            if (src.size() <= N) {
                m_Data = inline_ptr();
                m_Capacity = N;
            }
            else {
                m_Data = static_cast<T*>(operator new(src.size() * sizeof(T)));
                m_Capacity = src.size();
            }

            copy_range(m_Data, src.data(), src.size());
            m_Size = src.size();
        }

        SmallVec(std::initializer_list<T> list)
            : SmallVec(std::span<const T>(list.begin(), list.size())) {}

        template <std::input_iterator It>
        SmallVec(It first, It last) {
            m_Data = inline_ptr();
            m_Capacity = N;
            m_Size = 0;

            for (; first != last; ++first) {
                emplace_back(*first);
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

        T& operator[](size_type i) { return m_Data[i]; }
        const T& operator[](size_type i) const { return m_Data[i]; }

        T* data() { return m_Data; }
        const T* data() const { return m_Data; }

        operator std::span<T>() noexcept { return {m_Data, m_Size}; }
        operator std::span<const T>() const noexcept { return {m_Data, m_Size}; }

        iterator begin() { return m_Data; }
        iterator end() { return m_Data + m_Size; }
        const_iterator begin() const { return m_Data; }
        const_iterator end() const { return m_Data + m_Size; }

        [[nodiscard]] size_type size() const { return m_Size; }
        [[nodiscard]] size_type capacity() const { return m_Capacity; }
        [[nodiscard]] bool empty() const { return m_Size == 0; }

        void reserve(const size_type newCap) {
            if (newCap <= m_Capacity) {
                return;
            }

            grow_to(newCap);
        }

        void clear() {
            destroy_range(m_Data, m_Size);
            m_Size = 0;
        }

        template <typename... Args>
        T& emplace_back(Args&&... args) {
            if (m_Size >= m_Capacity) {
                grow();
            }

            T* ptr = m_Data + m_Size;
            new(ptr) T(std::forward<Args>(args)...);
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

        class back_insert_iterator {
        public:
            using iterator_category = std::output_iterator_tag;
            using difference_type = std::ptrdiff_t;
            using value_type = void;
            using pointer = void;
            using reference = void;

            explicit back_insert_iterator(SmallVec& vec)
                : m_Vec(&vec) {}

            back_insert_iterator& operator=(const T& value) {
                m_Vec->push_back(value);
                return *this;
            }

            back_insert_iterator& operator=(T&& value) {
                m_Vec->push_back(std::move(value));
                return *this;
            }

            back_insert_iterator& operator*() { return *this; }
            back_insert_iterator& operator++() { return *this; }
            back_insert_iterator operator++(int) { return *this; }

        private:
            SmallVec* m_Vec;
        };

        friend back_insert_iterator back_inserter(SmallVec& vec) {
            return back_insert_iterator(vec);
        };

        template <std::input_iterator It>
        iterator insert(iterator pos, It first, It last) {
            const size_type index = pos - begin();

            if constexpr (std::forward_iterator<It>) {
                const auto count = static_cast<size_type>(std::distance(
                    first,
                    last
                ));

                if (count == 0) {
                    return begin() + index;
                }

                reserve(m_Size + count);

                // move tail
                for (size_type i = m_Size; i > index; --i) {
                    new(m_Data + (i + count - 1)) T(std::move(m_Data[i - 1]));
                    m_Data[i - 1].~T();
                }

                // copy inserted range
                size_type i = 0;
                for (; first != last; ++first, ++i) {
                    new(m_Data + index + i) T(*first);
                }

                m_Size += count;
            }
            else {
                // fallback for pure input iterators
                for (; first != last; ++first) {
                    pos = insert(pos, *first);
                    ++pos;
                }
            }

            return begin() + index;
        }

        iterator insert(iterator pos, const T& value) {
            const size_type index = pos - begin();

            if (m_Size >= m_Capacity) {
                grow();
            }

            pos = begin() + index;

            for (size_type i = m_Size; i > index; --i) {
                new(m_Data + i) T(std::move(m_Data[i - 1]));
                m_Data[i - 1].~T();
            }

            new(pos) T(value);
            ++m_Size;

            return pos;
        }


        iterator insert(iterator pos, T&& value) {
            const size_type index = pos - begin();

            if (m_Size >= m_Capacity) {
                grow();
            }

            pos = begin() + index;

            for (size_type i = m_Size; i > index; --i) {
                new(m_Data + i) T(std::move(m_Data[i - 1]));
                m_Data[i - 1].~T();
            }

            new(pos) T(std::move(value));
            ++m_Size;

            return pos;
        }

        T& back() {
            return m_Data[m_Size - 1];
        }

        const T& back() const {
            return m_Data[m_Size - 1];
        }

        T& front() {
            return m_Data[0];
        }

        const T& front() const {
            return m_Data[0];
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

        void grow_to(const size_type newCap) {
            T* newData = static_cast<T*>(operator new(
                newCap * sizeof(T)
            ));

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
            }
            else {
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
            }
            else {
                m_Data = other.m_Data;
                m_Capacity = other.m_Capacity;
                other.m_Data = other.inline_ptr();
                other.m_Capacity = N;
            }

            m_Size = other.m_Size;
            other.m_Size = 0;
        }

        static void destroy_range(T* data, const size_type count) {
            if constexpr (!std::is_trivially_destructible_v<T>) {
                for (size_type i = 0; i < count; i++) {
                    data[i].~T();
                }
            }
        }

        static void move_range(T* dst, T* src, const size_type count) {
            if constexpr (std::is_trivially_copyable_v<T>) {
                std::memcpy(dst, src, count * sizeof(T));
            }
            else {
                for (size_type i = 0; i < count; ++i) {
                    new(&dst[i]) T(std::move(src[i]));
                }
            }
        }

        static void copy_range(T* dst, const T* src, const size_type count) {
            if constexpr (std::is_trivially_copyable_v<T>) {
                std::memcpy(dst, src, count * sizeof(T));
            }
            else {
                for (size_type i = 0; i < count; ++i) {
                    new(&dst[i]) T(src[i]);
                }
            }
        }

    private:
        alignas(T) unsigned char m_Inline[sizeof(T) * N]{};

        T* m_Data;
        size_type m_Size;
        size_type m_Capacity;
    };
}
