#pragma once

#include <map>

// can't compile ska without this... MSVC throws an exception that std doesn't contain std::out_of_range without this
#include <deque>
#include <stdexcept>
#include "ska/flat_hash_map.hpp"

#define QS_STRINGIFY_IMPL(x) #x
#define QS_STRINGIFY(x) QS_STRINGIFY_IMPL(x)

namespace Quelos {
	using byte = std::byte;
	consteval int GetBit(const int x) { return 1 << x; }

	template <typename TKey, typename TValue>
	using HashMap = ska::flat_hash_map<TKey, TValue>;

	template <typename TKey, typename TValue>
	using OrderedMap = std::map<TKey, TValue>;

	template <typename TKey, typename TValue>
	using HashSet = ska::flat_hash_set<TKey, TValue>;

	template <typename T>
	using Deque = std::deque<T>;

	template <typename T>
	using Vec = std::vector<T>;

	template <typename TFirst, typename  TSecond>
	using Pair = std::pair<TFirst, TSecond>;
}
