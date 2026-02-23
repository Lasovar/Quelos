#pragma once

#include "Camera.h"

namespace Quelos {
    class SceneCamera : public Camera {
    public:
        enum class ProjectionType : uint8_t { Perspective = 0, Orthographic = 1 };

    public:
        SceneCamera();
        ~SceneCamera() override;

        void SetViewportSize(uint32_t width, uint32_t height);

        void SetPerspective(float verticalFov, float nearClip, float farClip);

        [[nodiscard]] float GetPerspectiveVerticalFOV() const { return m_PerspectiveFOV; }
        [[nodiscard]] float GetPerspectiveNearClip() const { return m_PerspectiveNear; }
        [[nodiscard]] float GetPerspectiveFarClip() const { return m_PerspectiveFar; }

        void SetPerspectiveVerticalFOV(const float size) {
            m_PerspectiveFOV = size;
        }

        void SetPerspectiveNearClip(const float nearClip) {
            m_PerspectiveNear = nearClip;
        }

        void SetPerspectiveFarClip(const float farClip) {
            m_PerspectiveFar = farClip;
        }

        void SetOrthographic(float size, float nearClip, float farClip);

        [[nodiscard]] float GetOrthographicSize() const { return m_OrthographicSize; }
        [[nodiscard]] float GetOrthographicNearClip() const { return m_OrthographicNear; }
        [[nodiscard]] float GetOrthographicFarClip() const { return m_OrthographicFar; }

        void SetOrthographicSize(const float size) {
            m_OrthographicSize = size;
        }

        void SetOrthographicNearClip(const float nearClip) {
            m_OrthographicNear = nearClip;
        }

        void SetOrthographicFarClip(const float farClip) {
            m_OrthographicFar = farClip;
        }

        [[nodiscard]] ProjectionType GetProjectionType() const { return m_ProjectionType; }
        void SetProjectionType(const ProjectionType type) { m_ProjectionType = type; }

        void RecalculateProjection();

    private:
        ProjectionType m_ProjectionType = ProjectionType::Perspective;

        float m_PerspectiveFOV = 60.0f;
        float m_PerspectiveNear = 0.01f, m_PerspectiveFar = 1000.0f;

        float m_OrthographicSize = 10.0f;
        float m_OrthographicNear = -100.0f, m_OrthographicFar = 100.0f;

        float m_AspectRatio = 0.0f;
    };
}
