#pragma once

#include <vector>
#include <algorithm>

#include "DefaultComparer.h"
#include "Vector.hpp"

namespace Quelos {
    template <typename T, typename Compare = DefaultCompare<T>>
    class SortedVec {
    public:
        using container_type = Vec<T>;
        using iterator = typename container_type::iterator;
        using const_iterator = typename container_type::const_iterator;
        using allocator_type = typename container_type::allocator_type;

    public:
        SortedVec() = default;

        explicit SortedVec(std::pmr::memory_resource* memoryResource) : m_Data(memoryResource) {}
        explicit SortedVec(allocator_type allocator) : m_Data(allocator) {}
        explicit SortedVec(AllocatorType allocatorType) : m_Data(allocatorType) {}

        SortedVec(const SortedVec&) = delete;
        SortedVec& operator=(const SortedVec&) = delete;

        SortedVec(SortedVec&&) noexcept = default;
        SortedVec& operator=(SortedVec&&) = default;

        explicit SortedVec(SortedVec&& other, std::pmr::memory_resource* allocatorType)
            : m_Data(std::move(other.m_Data), allocatorType), m_Compare(other.m_Compare) {}

        explicit SortedVec(SortedVec&& other, const allocator_type& allocator)
            : SortedVec(std::move(other), allocator.resource()) {}

        explicit SortedVec(SortedVec&& other, const AllocatorType allocatorType)
            : SortedVec(std::move(other), GetAllocator(allocatorType)) {}

        void init(std::pmr::memory_resource* memoryResource) {
            m_Data.init(memoryResource);
        }

        void init(allocator_type allocator) {
            m_Data.init(allocator);
        }

        void init(AllocatorType allocatorType) {
            m_Data.init(allocatorType);
        }

        [[nodiscard]] SortedVec clone(std::pmr::memory_resource* memoryResource) {
            return SortedVec(m_Data.clone(memoryResource));
        }

        [[nodiscard]] SortedVec clone(const allocator_type& allocator) {
            return SortedVec(m_Data.clone(allocator));
        }

        [[nodiscard]] SortedVec clone(AllocatorType allocatorType) {
            return SortedVec(m_Data.clone(allocatorType));
        }

        iterator find(const T& value) {
            auto it = lower_bound(value);
            if (it != m_Data.end() && !m_Compare(value, *it)) {
                return it;
            }

            return m_Data.end();
        }

        const_iterator find(const T& value) const {
            auto it = lower_bound(value);
            if (it != m_Data.end() && !m_Compare(value, *it)) {
                return it;
            }

            return m_Data.end();
        }

        bool contains(const T& value) const {
            return find(value) != m_Data.end();
        }

        template <typename... Args>
        iterator emplace(Args&&... args) {
            T value(std::forward<Args>(args)...);

            auto it = std::lower_bound(m_Data.begin(), m_Data.end(), value, m_Compare);
            return m_Data.emplace(it, std::move(value));
        }

        iterator insert(const T& value) {
            auto it = lower_bound(value);

            return m_Data.insert(it, value);
        }

        iterator insert(T&& value) {
            auto it = lower_bound(value);

            return m_Data.insert(it, std::move(value));
        }

        void erase(const T& value) {
            auto it = find(value);
            if (it != m_Data.end()) {
                m_Data.erase(it);
            }
        }

        iterator begin() { return m_Data.begin(); }
        iterator end() { return m_Data.end(); }
        const_iterator begin() const { return m_Data.begin(); }
        const_iterator end() const { return m_Data.end(); }

        [[nodiscard]] size_t size() const { return m_Data.size(); }
        [[nodiscard]] bool empty() const { return m_Data.empty(); }
        void reserve(size_t n) { m_Data.reserve(n); }
        void clear() { m_Data.clear(); }

        T& operator[](size_t i) { return m_Data[i]; }
        const T& operator[](size_t i) const { return m_Data[i]; }

    private:
        explicit SortedVec(container_type&& data) : m_Data(data) {}

        iterator lower_bound(const T& value) {
            return std::lower_bound(m_Data.begin(), m_Data.end(), value, m_Compare);
        }

        const_iterator lower_bound(const T& value) const {
            return std::lower_bound(m_Data.begin(), m_Data.end(), value, m_Compare);
        }

    private:
        container_type m_Data;
        Compare m_Compare;
    };
}
