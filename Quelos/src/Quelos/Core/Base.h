#pragma once

#include  <memory>
#include <cstdint>

using byte = uint8_t;

namespace Quelos {
	constexpr int GetBit(const int x) { return 1 << x; }

	template<typename T>
	using Scope = std::unique_ptr<T>;
	template<typename T, typename ... Args>
	constexpr Scope<T> CreateScope(Args&& ... args)
	{
		return std::make_unique<T>(std::forward<Args>(args)...);
	}

}

