#include "SceneCamera.h"

#include "bgfx/bgfx.h"
#include "bx/math.h"
#include "glm/gtc/type_ptr.hpp"

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
            bx::mtxProj(
                glm::value_ptr(m_Projection),
                m_PerspectiveFOV,
                m_AspectRatio,
                m_PerspectiveNear,
                m_PerspectiveFar,
                bgfx::getCaps()->homogeneousDepth
            );
        }
        else
        {
            const float orthoLeft = -m_OrthographicSize * m_AspectRatio * 0.5f;
            const float orthoRight = m_OrthographicSize * m_AspectRatio * 0.5f;
            const float orthoBottom = -m_OrthographicSize * 0.5f;
            const float orthoTop = m_OrthographicSize * 0.5f;

            bx::mtxOrtho(
                glm::value_ptr(m_Projection),
                orthoLeft,
                orthoRight,
                orthoBottom,
                orthoTop,
                m_OrthographicNear,
                m_OrthographicFar,
                0.0f,
                bgfx::getCaps()->homogeneousDepth
            );
        }
    }
}
