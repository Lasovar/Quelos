#pragma once

#include <memory>

namespace Quelos {
    template <typename T>
    using UniquePtr = std::unique_ptr<T>;

    template <typename T, typename... Args>
    constexpr UniquePtr<T> CreateUnique(Args&&... args) {
        return std::make_unique<T>(std::forward<Args>(args)...);
    }

    template <class T1, class T2>
    [[nodiscard]] UniquePtr<T1> ScopeAs(UniquePtr<T2>&& scope) noexcept { return UniquePtr<T1>(std::move(scope)); }

    template <typename T>
    using SharedFromThis = std::enable_shared_from_this<T>;

    template <typename T>
    using SharedPtr = std::shared_ptr<T>;

    template <typename T, typename... Args>
    constexpr SharedPtr<T> CreateShared(Args&&... args) {
        return std::make_shared<T>(std::forward<Args>(args)...);
    }

    template <class T1, class T2>
    [[nodiscard]] SharedPtr<T1> SharedAs(const SharedPtr<T2>& ref) noexcept { return std::static_pointer_cast<T1>(ref); }

    template <typename T>
    using WeakPtr = std::weak_ptr<T>;

    template <typename T>
    struct IsShared : std::false_type { };

    template <typename T>
    struct IsShared<SharedPtr<T>> : std::true_type { };

    template <typename T>
    inline constexpr bool IsSharedV = IsShared<T>::value;

    template <typename T>
    struct SharedInner;

    template <typename T>
    struct SharedInner<SharedPtr<T>> { using Type = T; };

    template <typename T>
    using SharedInnerT = SharedInner<T>::Type;
}
