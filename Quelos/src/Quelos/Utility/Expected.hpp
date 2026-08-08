//
// Created by lasovar on 7/26/26.
//

#pragma once
#include <expected>

namespace Quelos {
    template <typename TExpected, typename TError>
    using Expected = std::expected<TExpected, TError>;

    template <typename TError>
    using Unexpected = std::unexpected<TError>;

    inline constexpr std::unexpect_t Unexpect{};
}
