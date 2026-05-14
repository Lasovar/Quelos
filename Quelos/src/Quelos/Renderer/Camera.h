#pragma once

#include "Quelos/Math/Math.h"

namespace Quelos {
    class QS_API Camera {
    public:
        Camera() = default;
        explicit Camera(const float4x4& projection) : m_Projection(projection) {}

        virtual ~Camera() = default;

        [[nodiscard]] const float4x4& GetProjection() const { return m_Projection; }
    protected:
        float4x4 m_Projection = float4x4::identity();
    };
}
