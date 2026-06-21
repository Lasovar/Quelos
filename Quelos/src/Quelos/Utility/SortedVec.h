#pragma once

#include <vector>
#include <algorithm>

#include "DefaultComparer.h"

namespace Quelos {
    template <typename T, typename Compare = DefaultCompare<T>>
    class SortedVec {
    public:
        using container_type = std::vector<T>;
        using iterator = typename container_type::iterator;
        using const_iterator = typename container_type::const_iterator;

    public:
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

        size_t size() const { return m_Data.size(); }
        bool empty() const { return m_Data.empty(); }
        void reserve(size_t n) { m_Data.reserve(n); }
        void clear() { m_Data.clear(); }

        T& operator[](size_t i) { return m_Data[i]; }
        const T& operator[](size_t i) const { return m_Data[i]; }

    private:
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
