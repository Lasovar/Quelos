//
// Created by lasovar on 7/26/26.
//

#pragma once

#include <optional>

namespace Quelos {
    template <typename T>
    using Optional = std::optional<T>;
    inline constexpr std::nullopt_t None { std::nullopt };
}
