#pragma once

#include <vector>
#include <algorithm>

#include "DefaultComparer.h"

namespace Quelos {

    template <typename K, typename V, typename Compare = DefaultCompare<K>>
    class FlatMap {
    public:
        using value_type = std::pair<K, V>;
        using container_type = std::vector<value_type>;
        using iterator = typename container_type::iterator;
        using const_iterator = typename container_type::const_iterator;

    public:
        iterator find(const K& key) {
            auto it = lower_bound(key);
            if (it != m_Data.end() && !m_Compare(key, it->first)) {
                return it;
            }

            return m_Data.end();
        }

        const_iterator find(const K& key) const {
            auto it = lower_bound(key);
            if (it != m_Data.end() && !m_Compare(key, it->first)) {
                return it;
            }

            return m_Data.end();
        }

        V& operator[](const K& key) {
            auto it = lower_bound(key);

            if (it == m_Data.end() || m_Compare(key, it->first)) {
                it = m_Data.insert(it, {key, V{}});
            }

            return it->second;
        }

        std::pair<iterator, bool> insert(const value_type& value) {
            auto it = lower_bound(value.first);

            if (it != m_Data.end() && !m_Compare(value.first, it->first))
                return {it, false};

            it = m_Data.insert(it, value);
            return {it, true};
        }

        template <typename... Args>
        std::pair<iterator, bool> emplace(K key, Args&&... args) {
            auto it = lower_bound(key);

            if (it != m_Data.end() && !m_Compare(key, it->first))
                return {it, false};

            it = m_Data.emplace(it, std::move(key), V(std::forward<Args>(args)...));
            return {it, true};
        }

        void erase(const K& key) {
            auto it = find(key);
            if (it != m_Data.end())
                m_Data.erase(it);
        }

        iterator begin() { return m_Data.begin(); }
        iterator end() { return m_Data.end(); }
        const_iterator begin() const { return m_Data.begin(); }
        const_iterator end() const { return m_Data.end(); }

        size_t size() const { return m_Data.size(); }
        bool empty() const { return m_Data.empty(); }
        void reserve(size_t n) { m_Data.reserve(n); }
        void clear() { m_Data.clear(); }

    private:
        iterator lower_bound(const K& key) {
            return std::lower_bound(
                m_Data.begin(), m_Data.end(), key,
                [this](const value_type& a, const K& b) {
                    return m_Compare(a.first, b);
                });
        }

        const_iterator lower_bound(const K& key) const {
            return std::lower_bound(
                m_Data.begin(), m_Data.end(), key,
                [this](const value_type& a, const K& b) {
                    return m_Compare(a.first, b);
                });
        }

    private:
        container_type m_Data;
        Compare m_Compare;
    };
}
