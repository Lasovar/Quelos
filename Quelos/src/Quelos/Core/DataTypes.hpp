//
// Created by lasovar on 7/11/26.
//

#pragma once

#include <deque>
#include <span>
#include <filesystem>

#include "ankerl/unordered_dense.h"

#include "Quelos/Utility/SortedVec.h"
#include "Quelos/Utility/SortedSet.h"
#include "Quelos/Utility/SmallVec.h"
#include "Quelos/Utility/Vec.hpp"
#include "Quelos/Utility/Span32.h"
#include "Quelos/Utility/FlatMap.h"
#include "Quelos/Utility/SmallVec.h"

namespace Quelos {
    using byte = std::byte;

    template <typename TKey, size_t N>
    using Array = std::array<TKey, N>;

    template <typename TKey, typename TValue>
    using HashMap = ankerl::unordered_dense::map<TKey, TValue>;

    template <typename TKey, typename TValue>
    using SegmentedMap = ankerl::unordered_dense::segmented_map<TKey, TValue>;

    template <typename TKey, typename TValue>
    using SortedMap = FlatMap<TKey, TValue>;

    template <typename TValue>
    using HashSet = ankerl::unordered_dense::set<TValue>;

    template <typename TValue>
    using SegmentedSet = ankerl::unordered_dense::segmented_set<TValue>;

    template <typename T>
    using Vec64 = Vec<T, uint64_t>;

    template <typename T>
    using Deque = std::deque<T>;

    template <typename T>
    using Span = std::span<T>;

    template <typename TFirst, typename TSecond>
    using Pair = std::pair<TFirst, TSecond>;

    using BufferView = Span<const byte>;
    using MutBufferView = Span<byte>;

    using OsPath = std::filesystem::path;

    template <typename T>
    using Option = std::optional<T>;
    inline constexpr std::nullopt_t None { std::nullopt };
}
