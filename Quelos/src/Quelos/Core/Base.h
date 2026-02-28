#pragma once

#include <flat_map>
#include "ska/flat_hash_map.hpp"

namespace Quelos {
	using byte = std::byte;
	consteval int GetBit(const int x) { return 1 << x; }

	template <typename TKey, typename TValue>
	using Map = ska::flat_hash_map<TKey, TValue>;

	template <typename TKey, typename TValue>
	using OrderedMap = std::flat_map<TKey, TValue>;

	template <typename TKey, typename TValue>
	using Set = ska::flat_hash_set<TKey, TValue>;

	template <typename T>
	using Vec = std::vector<T>;

	template <typename TFirst, typename  TSecond>
	using Pair = std::pair<TFirst, TSecond>;
}
