#include "Math.h"

#include "hlsl++.h"

#include "Quelos/Renderer/Renderer.h"

namespace Quelos::mathExt {
    float4x4 orthographic(
        const float left,
        const float right,
        const float bottom,
        const float top,
        const float zNear,
        const float zFar
    ) {
        using namespace math;

        const frustum cameraFrustum(left, right, bottom, top, zNear, zFar);
        return float4x4::orthographic(
            projection(
                cameraFrustum,
                Renderer::HomogenousDepth() ? zclip::zero : zclip::minus_one
            )
        );
    }

    float4x4 perspective(const float fovRad, const float aspectRatio, const float nearClip, const float farClip) {
        using namespace math;

        const frustum cameraFrustum = frustum::field_of_view_y(fovRad, aspectRatio, nearClip, farClip);
        return float4x4::perspective(
            projection(
                cameraFrustum,
                Renderer::HomogenousDepth() ? zclip::zero : zclip::minus_one
            )
        );
    }

    float4x4 view(const quaternion& rotation, const float3& position) {
        using namespace math;
        return inverse(mul(float4x4::translation(position), float4x4(rotation)));
    }

    float4x4 view(const float4x4& world) {
        return math::inverse(world);
    }

    float4x4 srt(const float3& scale, const quaternion& rotation, const float3& position) {
        using namespace math;
        return mul(
            float4x4::scale(scale),
            mul(
                float4x4(rotation),
                float4x4::translation(position)
            )
        );
    }
}
