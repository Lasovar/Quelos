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
#include "Quelos/Utility/SmallVec.hpp"
#include "Quelos/Utility/Vector.hpp"
#include "Quelos/Utility/Span32.h"
#include "Quelos/Utility/FlatMap.h"
#include "Quelos/Utility/Pair.hpp"
#include "Quelos/Utility/SmallVec.hpp"
#include "Quelos/Utility/HashMap.hpp"
#include "Quelos/Utility/HashSet.hpp"
#include "Quelos/Utility/ReferenceWrapper.hpp"
#include "Quelos/Utility/Optional.hpp"
#include "Quelos/Utility/Expected.hpp"
#include "Quelos/Utility/StringView.hpp"
#include "Quelos/Utility/String.hpp"

namespace Quelos {
    using byte = std::byte;

    template <typename TKey, size_t N>
    using Array = std::array<TKey, N>;

    template <typename TKey, typename TValue>
    using SegmentedMap = ankerl::unordered_dense::segmented_map<TKey, TValue>;

    template <typename TKey, typename TValue>
    using SortedMap = FlatMap<TKey, TValue>;

    template <typename TValue>
    using SegmentedSet = ankerl::unordered_dense::segmented_set<TValue>;

    template <typename T>
    using Deque = std::deque<T>;

    template <typename T, uint64_t Extent = k_DynamicExtent64>
    using Span64 = SpanT<T, uint64_t, Extent>;

    template <typename T, uint32_t Extent = k_DynamicExtent32>
    using Span32 = SpanT<T, uint32_t, Extent>;

    template <typename T, uint32_t Extent = k_DynamicExtent32>
    using Span = Span32<T, Extent>;

    using BufferView = Span64<const byte>;
    using MutBufferView = Span64<byte>;

    using OsPath = std::filesystem::path;
}
