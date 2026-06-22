//
// Created by lasovar on 6/22/26.
//

#pragma once

#include "hlsl++.h"

namespace Quelos::math::interop {
    struct float3 {
        float3() = default;
        float3(const float v) : x(v), y(v), z(v) {}
        float3(const float f1, const float f2, const float f3) : x(f1), y(f2), z(f3) {}
        float3(const hlslpp::float3& f) { hlslpp::store(&x, f); }

        float x = 0, y = 0, z = 0;

        float3 operator+(const float3& v) const { return float3(x + v.x, y + v.y, z + v.z); }
        float3 operator-(const float3& v) const { return float3(x - v.x, y - v.y, z - v.z); }
        float3 operator*(const float v) const { return float3(x * v, y * v, z * v); }

        explicit operator hlslpp::float3() { return { x, y, z }; }
     };
}
