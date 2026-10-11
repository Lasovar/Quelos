//
// Created by lasovar on 7/26/26.
//

#pragma once

#include <optional>

namespace Quelos {
    template <typename T>
    using Optional = std::optional<T>;
    inline constexpr std::nullopt_t None { std::nullopt };

    template <typename T>
    struct IsOptional : std::false_type {};

    template <typename T>
    struct IsOptional<Optional<T>> : std::true_type {};

    template <typename T>
    inline constexpr bool IsOptionalV = IsOptional<std::remove_cvref_t<T>>::value;
}
