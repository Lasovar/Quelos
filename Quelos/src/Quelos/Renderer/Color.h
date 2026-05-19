#pragma once

#include "Quelos/Math/Math.h"

namespace Quelos {
    struct Color : float4 {
        constexpr Color() = default;
        inline Color(const float4& f) : float4(f) {}
        inline Color(const float r, const float g, const float b, const float a = 1.0f)
            : float4(r, g, b, a) {}

        [[nodiscard]]
        static Color FromBytes(
            const uint8_t r,
            const uint8_t g,
            const uint8_t b,
            const uint8_t a = 255
        ) {
            constexpr float inv = 1.0f / 255.0f;

            return {
                static_cast<float>(r) * inv,
                static_cast<float>(g) * inv,
                static_cast<float>(b) * inv,
                static_cast<float>(a) * inv
            };
        }

        [[nodiscard]]
        constexpr uint32_t ToRGBA8() const {
            auto clamp = [](const float v) constexpr -> uint32_t {
                return static_cast<uint32_t>(
                    v <= 0.0f ? 0 :
                    v >= 1.0f ? 255 :
                    v * 255.0f + 0.5f
                );
            };

            return
                clamp(r) << 24 |
                clamp(g) << 16 |
                clamp(b) << 8  |
                clamp(a);
        }

        [[nodiscard]]
        constexpr const float* value_ptr() const {
            return f32;
        }

        [[nodiscard]]
        constexpr float* value_ptr() {
            return f32;
        }

        static Color White() { return {1,1,1,1}; }
        static Color Black() { return {0,0,0,1}; }
        static Color Red()   { return {1,0,0,1}; }
        static Color Green() { return {0,1,0,1}; }
        static Color Blue()  { return {0,0,1,1}; }
    };
}
