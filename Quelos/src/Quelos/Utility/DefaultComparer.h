#pragma once

namespace Quelos {
    template <typename K>
    struct DefaultCompare {
        using is_transparent = void;

        template <typename A, typename B>
        constexpr bool operator()(const A& a, const B& b) const {
            return a < b;
        }
    };
}
