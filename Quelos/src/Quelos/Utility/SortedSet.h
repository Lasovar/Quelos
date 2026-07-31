#pragma once

#include <algorithm>

#include "DefaultComparer.h"

namespace Quelos {
    template <typename T, typename Compare = DefaultCompare<T>>
    class SortedSet {
    public:
        using container_type = Vec64<T>;
        using iterator = typename container_type::iterator;
        using const_iterator = typename container_type::const_iterator;
        using allocator_type = typename container_type::allocator_type;

    public:
        SortedSet() = default;

        explicit SortedSet(std::pmr::memory_resource* memoryResource) : m_Data(memoryResource) {}
        explicit SortedSet(allocator_type allocator) : m_Data(allocator) {}
        explicit SortedSet(AllocatorType allocatorType) : m_Data(allocatorType) {}

        SortedSet(const SortedSet&) = delete;
        SortedSet& operator=(const SortedSet&) = delete;

        SortedSet(SortedSet&&) noexcept = default;
        SortedSet& operator=(SortedSet&&) = default;

        explicit SortedSet(SortedSet&& other, std::pmr::memory_resource* allocatorType)
            : m_Data(std::move(other.m_Data), allocatorType), m_Compare(other.m_Compare) {}

        explicit SortedSet(SortedSet&& other, const allocator_type& allocator)
            : SortedSet(std::move(other), allocator.resource()) {}

        explicit SortedSet(SortedSet&& other, const AllocatorType allocatorType)
            : SortedSet(std::move(other), GetAllocator(allocatorType)) {}

        void init(std::pmr::memory_resource* memoryResource) {
            m_Data.init(memoryResource);
        }

        void init(allocator_type allocator) {
            m_Data.init(allocator);
        }

        void init(AllocatorType allocatorType) {
            m_Data.init(allocatorType);
        }

        [[nodiscard]] SortedSet clone(std::pmr::memory_resource* memoryResource) {
            return SortedSet(m_Data.clone(memoryResource));
        }

        [[nodiscard]] SortedSet clone(const allocator_type& allocator) {
            return SortedVec(m_Data.clone(allocator));
        }

        [[nodiscard]] SortedSet clone(AllocatorType allocatorType) {
            return SortedSet(m_Data.clone(allocatorType));
        }


        iterator find(const T& value) {
            auto it = lower_bound(value);
            if (it != m_Data.end() && equals(*it, value)) {
                return it;
            }

            return m_Data.end();
        }

        const_iterator find(const T& value) const {
            auto it = lower_bound(value);
            if (it != m_Data.end() && equals(*it, value)) {
                return it;
            }

            return m_Data.end();
        }

        bool contains(const T& value) const {
            return find(value) != m_Data.end();
        }

        template <typename... Args>
        std::pair<iterator, bool> emplace(Args&&... args) {
            auto value = std::make_from_tuple<T>(
                std::uses_allocator_construction_args<T>(
                    m_Data.get_allocator(),
                    std::forward<Args>(args)...
                )
            );

            auto it = std::lower_bound(m_Data.begin(), m_Data.end(), value, m_Compare);
            if (it != m_Data.end() && equals(*it, value)) {
                return {it, false};
            }

            it = m_Data.emplace(it, std::move(value));
            return {it, true};
        }

        bool insert(const T& value) {
            auto it = lower_bound(value);

            if (it != m_Data.end() && equals(*it, value)) {
                return false;
            }

            m_Data.insert(it, value);
            return true;
        }

        bool insert(T&& value) {
            auto it = lower_bound(value);

            if (it != m_Data.end() && equals(*it, value)) {
                return false;
            }

            m_Data.insert(it, std::move(value));
            return true;
        }

        bool erase(const T& value) {
            auto it = find(value);
            if (it != m_Data.end()) {
                m_Data.erase(it);
                return true;
            }

            return false;
        }

        void erase(iterator it) {
            m_Data.erase(it);
        }

        iterator begin() { return m_Data.begin(); }
        iterator end() { return m_Data.end(); }
        const_iterator begin() const { return m_Data.begin(); }
        const_iterator end() const { return m_Data.end(); }

        [[nodiscard]] size_t size() const { return m_Data.size(); }
        [[nodiscard]] bool empty() const { return m_Data.empty(); }

        void reserve(size_t n) { m_Data.reserve(n); }
        void clear() { m_Data.clear(); }

    private:
        iterator lower_bound(const T& value) {
            return std::lower_bound(m_Data.begin(), m_Data.end(), value, m_Compare);
        }

        const_iterator lower_bound(const T& value) const {
            return std::lower_bound(m_Data.begin(), m_Data.end(), value, m_Compare);
        }

        bool equals(const T& a, const T& b) const {
            return !m_Compare(a, b) && !m_Compare(b, a);
        }

    private:
        container_type m_Data;
        Compare m_Compare;
    };
}
