#pragma once

#include <memory>

namespace Quelos {
	template<typename T>
	using Scope = std::unique_ptr<T>;
	template<typename T, typename ... Args>
	constexpr Scope<T> CreateScope(Args&& ... args) {
		return std::make_unique<T>(std::forward<Args>(args)...);
	}

	template<typename T>
	using RefCounted = std::enable_shared_from_this<T>;

	template<typename T>
	using Ref = std::shared_ptr<T>;

	template<typename T, typename ... Args>
	constexpr Ref<T> CreateRef(Args&& ... args) {
		return std::make_shared<T>(std::forward<Args>(args)...);
	}

	template<class T1, class T2>
	[[nodiscard]] Ref<T1> RefAs(const Ref<T2>& ref) noexcept { return std::static_pointer_cast<T1>(ref); }
}
