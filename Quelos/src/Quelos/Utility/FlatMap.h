#pragma once

#include <vector>
#include <algorithm>

#include "DefaultComparer.h"
#include "Optional.hpp"
#include "Pair.hpp"

namespace Quelos {
    template <typename Key, typename Value, typename Compare = DefaultCompare<Key>>
    class FlatMap {
    public:
        using value_type = Pair<Key, Value>;
        using container_type = Vec64<value_type>;
        using iterator = typename container_type::iterator;
        using const_iterator = typename container_type::const_iterator;
        using allocator_type = typename container_type::allocator_type;

    public:
        FlatMap() = default;
        FlatMap(const FlatMap&) = delete;
        FlatMap& operator=(const FlatMap&) = delete;
        FlatMap(FlatMap&&) = default;
        FlatMap& operator=(FlatMap&&) = default;

        explicit FlatMap(const allocator_type allocator)
            : m_Data(allocator) {}

        explicit FlatMap(const AllocatorType allocatorType)
            : m_Data(allocatorType) {}

        explicit FlatMap(FlatMap&& other, const allocator_type allocator)
            : m_Data(std::move(other.m_Data), allocator) {}

        explicit FlatMap(FlatMap&& other, const AllocatorType allocatorType)
            : m_Data(std::move(other.m_Data), allocatorType) {}

        void init(const allocator_type allocator) {
            m_Data.init(allocator);
        }

        void init(const AllocatorType allocatorType) {
            m_Data.init(allocatorType);
        }

        iterator find(const Key& key) {
            auto it = lower_bound(key);
            if (it != m_Data.end() && !m_Compare(key, it->first)) {
                return it;
            }

            return m_Data.end();
        }

        const_iterator find(const Key& key) const {
            auto it = lower_bound(key);
            if (it != m_Data.end() && !m_Compare(key, it->first)) {
                return it;
            }

            return m_Data.end();
        }

        template <typename K = Key>
        Value& operator[](const K& key) {
            auto it = lower_bound(key);

            if (it == m_Data.end() || m_Compare(key, it->first)) {
                it = m_Data.insert(
                    it,
                    std::make_from_tuple<Pair<Key, Value>>(
                        std::uses_allocator_construction_args<Pair<Key, Value>>(
                            m_Data.get_allocator(),
                            std::piecewise_construct,
                            std::forward_as_tuple(key),
                            std::forward_as_tuple()
                        )
                    )
                );
            }

            return it->second;
        }

        template <typename K = Key>
        Value& operator[](K&& key) {
            auto it = lower_bound(key);

            if (it == m_Data.end() || m_Compare(key, it->first)) {
                it = m_Data.insert(
                    it,
                    std::make_from_tuple<Pair<Key, Value>>(
                        std::uses_allocator_construction_args<Pair<Key, Value>>(
                            m_Data.get_allocator(),
                            std::piecewise_construct,
                            std::forward_as_tuple(std::forward<K>(key)),
                            std::forward_as_tuple()
                        )
                    )
                );
            }

            return it->second;
        }

        template <typename K = Key>
        Optional<Ref<Value>> at(const K& key) {
            auto it = lower_bound(key);

            if (it == m_Data.end()) {
                return None;
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
        std::pair<iterator, bool> emplace(Key key, Args&&... args) {
            auto it = lower_bound(key);

            if (it != m_Data.end() && !m_Compare(key, it->first)) {
                return {it, false};
            }

            it = m_Data.emplace(it, std::move(key), Value(std::forward<Args>(args)...));
            return {it, true};
        }

        template <class... Args>
        auto try_emplace(const Key& key, Args&&... args) {
            auto it = lower_bound(key);
            if (it != end() && it->first == key) {
                return std::pair{it, false};
            }

            using Mapped = Value;

            auto mapped = std::make_from_tuple<Mapped>(
                std::uses_allocator_construction_args<Mapped>(
                    m_Data.get_allocator(),
                    std::forward<Args>(args)...
                )
            );

            it = m_Data.emplace(it, key, std::move(mapped));

            return std::pair{it, true};
        }

        void erase(const Key& key) {
            auto it = find(key);
            if (it != m_Data.end())
                m_Data.Erase(it);
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
        template <typename K = Key>
        iterator lower_bound(const K& key) {
            return std::lower_bound(
                m_Data.begin(),
                m_Data.end(),
                key,
                [this](const value_type& a, const K& b) {
                    return m_Compare(a.first, b);
                });
        }

        template <typename K = Key>
        const_iterator lower_bound(const K& key) const {
            return std::lower_bound(
                m_Data.begin(),
                m_Data.end(),
                key,
                [this](const value_type& a, const K& b) {
                    return m_Compare(a.first, b);
                });
        }

    private:
        container_type m_Data;
        Compare m_Compare;
    };
}
