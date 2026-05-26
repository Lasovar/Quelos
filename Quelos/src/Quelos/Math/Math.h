#pragma once

#include "hlsl++.h"
#include "Quelos/Core/Base.h"
#include <cmath>

namespace Quelos {
    using int1 = hlslpp::int1;
    using int2 = hlslpp::int2;
    using int3 = hlslpp::int3;
    using int4 = hlslpp::int4;

    using uint1 = hlslpp::uint1;
    using uint2 = hlslpp::uint2;
    using uint3 = hlslpp::uint3;
    using uint4 = hlslpp::uint4;

    using float1 = hlslpp::float1;
    using float2 = hlslpp::float2;
    using float3 = hlslpp::float3;
    using float4 = hlslpp::float4;

    using float3x4 = hlslpp::float3x4;
    using float4x3 = hlslpp::float4x3;
    using float3x3 = hlslpp::float3x3;
    using float4x4 = hlslpp::float4x4;

    using double1 = hlslpp::double1;
    using double2 = hlslpp::double2;
    using double3 = hlslpp::double3;
    using double4 = hlslpp::double4;

    using quaternion = hlslpp::quaternion;

    // Portable/Interop
    using pint1 = hlslpp::interop::int1;
    using pint2 = hlslpp::interop::int2;
    using pint3 = hlslpp::interop::int3;
    using pint4 = hlslpp::interop::int4;
    using puint1 = hlslpp::interop::uint1;
    using puint2 = hlslpp::interop::uint2;
    using puint3 = hlslpp::interop::uint3;
    using puint4 = hlslpp::interop::uint4;

    using pfloat1 = hlslpp::interop::float1;
    using pfloat2 = hlslpp::interop::float2;
    using pfloat3 = hlslpp::interop::float3;
    using pfloat4 = hlslpp::interop::float4;

    using pfloat3x4 = hlslpp::interop::float3x4;
    using pfloat4x3 = hlslpp::interop::float4x3;
    using pfloat4x4 = hlslpp::interop::float4x4;

    using pdouble1 = hlslpp::interop::double1;
    using pdouble2 = hlslpp::interop::double2;
    using pdouble3 = hlslpp::interop::double3;
    using pdouble4 = hlslpp::interop::double4;

    using pquaternion = hlslpp::interop::float4;

    namespace math {
        constexpr float f_pi = 3.14159265358979323846f;
        constexpr float f_2pi = 2.0f * f_pi;
        constexpr float f_pi2 = f_pi / 2.0f;
        constexpr float f_pi4 = f_pi / 4.0f;

        constexpr double d_pi = 3.14159265358979323846264338327950288;
        constexpr double d_2pi = 2.0 * d_pi;
        constexpr double d_pi2 = d_pi / 2.0;
        constexpr double d_pi4 = d_pi / 4.0;

        using hlslpp::frustum;
        using hlslpp::projection;

        namespace zclip = hlslpp::zclip;

        using hlslpp::radians;
        constexpr float radians(const float f) { return f * (f_pi / 180.0f); }
        constexpr double radians(const double f) { return f * (d_pi / 180.0); }

        using hlslpp::degrees;
        constexpr float degrees(const float f) { return f * (180.0f / f_pi); }
        constexpr double degrees(const double f) { return f * (180.0 / d_pi); }

        using hlslpp::atan2;

        using hlslpp::cos;
        inline float cos(const float f) { return std::cos(f); }
        inline double cos(const double f) { return std::cos(f); }

        using hlslpp::sin;
        inline float sin(const float f) { return std::sin(f); }
        inline double sin(const double f) { return std::sin(f); }

        using hlslpp::asin;
        inline float asin(const float f) { return std::asin(f); }
        inline double asin(const double f) { return std::asin(f); }

        using hlslpp::sign;
        constexpr float sign(const float f) { return (f > 0.0f) - (f < 0.0f); }
        constexpr double sign(const double f) { return (f > 0.0) - (f < 0.0); }

        using hlslpp::min;
        using hlslpp::max;

        using hlslpp::clamp;
        inline float clamp(const float f, const float minf, const float maxf) { return min(max(f, minf), maxf); }
        inline double clamp(const double f, const double minf, const double maxf) { return min(max(f, minf), maxf); }

        using hlslpp::saturate;
        inline float saturate(const float f) { return clamp(f, 0.0f, 1.0f); }
        inline double saturate(const double f) { return clamp(f, 0.0, 1.0); }

        using hlslpp::length;
        using hlslpp::normalize;

        using hlslpp::any;
        using hlslpp::all;

        inline bool approximately(const float& a, const float& b, const float epsilon = 0.0001f) {
            return abs(a - b) < epsilon;
        }

        inline bool approximately(const float1& a, const float1& b, const float1& epsilon = 0.0001f) {
            return all(abs(a - b) < epsilon);
        }

        inline bool approximately(const float2& a, const float2& b, const float epsilon = 0.0001f) {
            return all(abs(a - b) < epsilon);
        }

        inline bool approximately(const float3& a, const float3& b, const float epsilon = 0.0001f) {
            return all(abs(a - b) < epsilon);
        }

        inline bool approximately(const float4& a, const float4& b, const float epsilon = 0.0001f) {
            return all(abs(a - b) < epsilon);
        }

        inline float roll(const quaternion& q) {
            const float1 y = 2.0f * (q.x * q.y + q.w * q.z);
            const float1 x = q.w * q.w + q.x * q.x - q.y * q.y - q.z * q.z;

            if(all(float2(x, y) == float2(0) == float2(std::numeric_limits<float>::epsilon()))) //avoid atan2(0,0) - handle singularity - Matiis
            {
                return 0.0f;
            }

            return atan2(y, x);
        }

        inline float pitch(const quaternion& q) {
            const float1 y = 2.0f * (q.y * q.z + q.w * q.x);
            const float1 x = q.w * q.w - q.x * q.x - q.y * q.y + q.z * q.z;

            if(all(float2(x, y) == float2(0) == std::numeric_limits<float>::epsilon())) //avoid atan2(0,0) - handle singularity - Matiis
            {
                return 2.0f * atan2(q.x, q.w);
            }

            return atan2(y, x);
        }

        inline float yaw(const quaternion& q) {
            return asin(clamp(-2.0f * (q.x * q.z - q.w * q.y), float1(-1.0f), float1(1.0f)));
        }

        inline float3 euler(const quaternion& f) {
            return {pitch(f), yaw(f), roll(f)};
        }

        using hlslpp::mul;
        using hlslpp::inverse;
        using hlslpp::transpose;

        /// @tparam T the multi component type
        /// @return the number of components in a type
        template <typename T>
        consteval uint32_t count() = delete;

        template <>
        consteval uint32_t count<int1>() { return 1; }
        template<>
        consteval uint32_t count<int2>() { return 2; }
        template<>
        consteval uint32_t count<int3>() { return 3; }
        template<>
        consteval uint32_t count<int4>() { return 4; }

        template <>
        consteval uint32_t count<uint1>() { return 1; }
        template<>
        consteval uint32_t count<uint2>() { return 2; }
        template<>
        consteval uint32_t count<uint3>() { return 3; }
        template<>
        consteval uint32_t count<uint4>() { return 4; }

        template<>
        consteval uint32_t count<float1>() { return 1; }
        template<>
        consteval uint32_t count<float2>() { return 2; }
        template<>
        consteval uint32_t count<float3>() { return 3; }
        template<>
        consteval uint32_t count<float4>() { return 4; }

        template<>
        consteval uint32_t count<float3x3>() { return 9; }
        template<>
        consteval uint32_t count<float3x4>() { return 12; }
        template<>
        consteval uint32_t count<float4x3>() { return 12; }
        template<>
        consteval uint32_t count<float4x4>() { return 16; }

        template <>
        consteval uint32_t count<double1>() { return 1; }
        template<>
        consteval uint32_t count<double2>() { return 2; }
        template<>
        consteval uint32_t count<double3>() { return 3; }
        template<>
        consteval uint32_t count<double4>() { return 4; }

        template<>
        consteval uint32_t count<quaternion>() { return 4; }

        using hlslpp::store;
        
        constexpr int32_t* value_ptr(int1& f) { return f.i32; }
        constexpr int32_t* value_ptr(int2& f) { return f.i32; }
        constexpr int32_t* value_ptr(int3& f) { return f.i32; }
        constexpr int32_t* value_ptr(int4& f) { return f.i32; }

        constexpr const int32_t* value_ptr(const int1& f) { return f.i32; }
        constexpr const int32_t* value_ptr(const int2& f) { return f.i32; }
        constexpr const int32_t* value_ptr(const int3& f) { return f.i32; }
        constexpr const int32_t* value_ptr(const int4& f) { return f.i32; }

        constexpr uint32_t* value_ptr(uint1& f) { return f.u32; }
        constexpr uint32_t* value_ptr(uint2& f) { return f.u32; }
        constexpr uint32_t* value_ptr(uint3& f) { return f.u32; }
        constexpr uint32_t* value_ptr(uint4& f) { return f.u32; }

        constexpr const uint32_t* value_ptr(const uint1& f) { return f.u32; }
        constexpr const uint32_t* value_ptr(const uint2& f) { return f.u32; }
        constexpr const uint32_t* value_ptr(const uint3& f) { return f.u32; }
        constexpr const uint32_t* value_ptr(const uint4& f) { return f.u32; }

        constexpr float* value_ptr(float1& f) { return f.f32; }
        constexpr float* value_ptr(float2& f) { return f.f32; }
        constexpr float* value_ptr(float3& f) { return f.f32; }
        constexpr float* value_ptr(float4& f) { return f.f32; }

        constexpr const float* value_ptr(const float1& f) { return f.f32; }
        constexpr const float* value_ptr(const float2& f) { return f.f32; }
        constexpr const float* value_ptr(const float3& f) { return f.f32; }
        constexpr const float* value_ptr(const float4& f) { return f.f32; }

        constexpr double* value_ptr(double1& f) { return f.f64; }
        constexpr double* value_ptr(double2& f) { return f.f64; }
        constexpr double* value_ptr(double3& f) { return f.f64; }
        constexpr double* value_ptr(double4& f) { return f.f64; }

        constexpr const double* value_ptr(const double1& f) { return f.f64; }
        constexpr const double* value_ptr(const double2& f) { return f.f64; }
        constexpr const double* value_ptr(const double3& f) { return f.f64; }
        constexpr const double* value_ptr(const double4& f) { return f.f64; }

        constexpr float* value_ptr(quaternion& f) { return f.f32; }
        constexpr const float* value_ptr(const quaternion& f) { return f.f32; }

        constexpr float* value_ptr(float3x4& f) { return f.f32_0; }
        constexpr float* value_ptr(float4x3& f) { return f.f32_0; }
        constexpr float* value_ptr(float4x4& f) { return &f[0][0]; }

        constexpr const float* value_ptr(const float3x4& f) { return f.f32_0; }
        constexpr const float* value_ptr(const float4x3& f) { return f.f32_0; }
        constexpr const float* value_ptr(const float4x4& f) { return &f[0][0]; }
    }

    namespace mathExt {
        QS_API float4x4 orthographic(float left, float right, float bottom, float top, float zNear, float zFar);
        QS_API float4x4 perspective(float fovRad, float aspectRatio, float nearClip, float farClip);
        QS_API float4x4 view(const float4x4& world);
        QS_API float4x4 view(const quaternion& rotation, const float3& position);
        QS_API float4x4 srt(const float3& scale, const quaternion& rotation, const float3& position);
    }
}
