#include "SceneCamera.h"

#include "bgfx/bgfx.h"

namespace Quelos {
    SceneCamera::SceneCamera() { RecalculateProjection(); }

    SceneCamera::~SceneCamera() {
    }

    void SceneCamera::SetPerspective(const float verticalFov, const float nearClip, const float farClip) {
        m_ProjectionType = ProjectionType::Perspective;

        m_PerspectiveFOV = verticalFov;
        m_PerspectiveNear = nearClip;
        m_PerspectiveFar = farClip;

        RecalculateProjection();
    }

    void SceneCamera::SetOrthographic(const float size, const float nearClip, const float farClip) {
        m_ProjectionType = ProjectionType::Orthographic;

        m_OrthographicSize = size;
        m_OrthographicNear = nearClip;
        m_OrthographicFar = farClip;

        RecalculateProjection();
    }

    void SceneCamera::SetViewportSize(const uint32_t width, const uint32_t height) {
        m_AspectRatio = static_cast<float>(width) / static_cast<float>(height);
        RecalculateProjection();
    }

    void SceneCamera::RecalculateProjection() {
        if (m_ProjectionType == ProjectionType::Perspective) {
            m_Projection = Math::PerspectiveMatrix(
                glm::radians(m_PerspectiveFOV),
                m_AspectRatio,
                m_PerspectiveNear,
                m_PerspectiveFar
            );
        }
        else {
            const float orthoLeft = -m_OrthographicSize * m_AspectRatio * 0.5f;
            const float orthoRight = m_OrthographicSize * m_AspectRatio * 0.5f;
            const float orthoBottom = -m_OrthographicSize * 0.5f;
            const float orthoTop = m_OrthographicSize * 0.5f;

            m_Projection = Math::OrthographicMatrix(
                orthoLeft,
                orthoRight,
                orthoBottom,
                orthoTop,
                m_OrthographicNear,
                m_OrthographicFar
            );
        }
    }
}
