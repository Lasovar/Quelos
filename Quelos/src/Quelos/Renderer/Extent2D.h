#pragma once

#include <cstdint>

namespace Quelos {
    struct Extent2D {
        uint32_t Width = 0;
        uint32_t Height = 0;

        [[nodiscard]] uint32_t Area() const { return Width * Height; }
        [[nodiscard]] float Aspect() const { return static_cast<float>(Width) / static_cast<float>(Height); }

        bool operator==(const Extent2D&) const = default;
    };
}
