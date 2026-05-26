#include "qspch.h"
#include "EditorCamera.h"

#include "Quelos/Core/Events/InputEvents.h"

#include "Quelos/Core/Application.h"

#include "Quelos/Math/Math.h"

namespace Quelos {
    EditorCamera::EditorCamera(const float fov, const float aspectRatio, const float nearClip, const float farClip)
        : Camera(mathExt::perspective(math::radians(fov), aspectRatio, nearClip, farClip)),
          m_FOV(fov), m_AspectRatio(aspectRatio), m_NearClip(nearClip), m_FarClip(farClip) {
        UpdateView();
        MousePan(float2{0.01f});
    }

    void EditorCamera::UpdateProjection() {
        m_AspectRatio = m_ViewportWidth / m_ViewportHeight;

        m_Projection = mathExt::perspective(
            math::radians(m_FOV),
            m_AspectRatio,
            m_NearClip,
            m_FarClip
        );
    }

    void EditorCamera::UpdateView() {
        if (m_LockRotation) {
            m_Yaw = m_Pitch = 0.0f; // Lock the camera's rotation
        }

        const float3 forward = {
            math::cos(m_Pitch) * math::sin(m_Yaw),
            math::sin(m_Pitch),
            math::cos(m_Pitch) * math::cos(m_Yaw)
        };

        m_ViewMatrix = float4x4::look_at(
            m_Position,
            m_Position + forward,
            float3(0.0f, 1.0f, 0.0f)
        );
    }

    float2 EditorCamera::PanSpeed() const {
        const float x = math::min(m_ViewportWidth / 1000.0f, 2.4f); // max = 2.4f
        const float xFactor = 0.0366f * (x * x) - 0.1778f * x + 0.3021f;

        const float y = math::min(m_ViewportHeight / 1000.0f, 2.4f); // max = 2.4f
        const float yFactor = 0.0366f * (y * y) - 0.1778f * y + 0.3021f;

        return { xFactor, yFactor };
    }

    float EditorCamera::RotationSpeed() {
        return 0.8f;
    }

    float EditorCamera::ZoomSpeed() const {
        float distance = m_Distance * 0.2f;
        distance = std::max(distance, 0.0f);
        float speed = distance * distance;
        speed = std::min(speed, 100.0f); // max speed = 100
        return speed;
    }

    void EditorCamera::OnUpdate(float deltaTime) {
        // Mouse smoothing
        float2 smoothed = m_MouseDelta;
        m_MouseDelta = float2(0.0f);

        float3 input{0.0f};

        if (m_RMB) {
            m_Yaw += smoothed.x * m_MouseSensitivity;
            m_Pitch += smoothed.y * m_MouseSensitivity;

            m_Pitch = math::clamp(m_Pitch, math::radians(-89.0f), math::radians(89.0f));

            // Direction vectors
            float3 forwardDir{
                std::cos(m_Pitch) * std::sin(m_Yaw),
                std::sin(m_Pitch),
                std::cos(m_Pitch) * std::cos(m_Yaw)
            };

            float3 rightDir = normalize(cross(float3(0, 1, 0), forwardDir));
            float3 upDir = normalize(cross(rightDir, forwardDir));

            if (m_Forward) input += forwardDir;
            if (m_Backwards) input -= forwardDir;
            if (m_Right) input += rightDir;
            if (m_Left) input -= rightDir;
            if (m_Up) input += upDir;
            if (m_Down) input -= upDir;

            if (math::length(input).x > 0.0f) {
                input = normalize(input);
            }
        }

        // Acceleration + damping
        m_Velocity += input * m_AspectRatio * deltaTime;
        m_Velocity -= m_Velocity * m_Damping * deltaTime;

        m_Position += m_Velocity * m_MoveSpeed * deltaTime;

        UpdateView();
    }

    void EditorCamera::OnEvent(Event& e) {
        EventDispatcher dispatcher(e);
        /*dispatcher.Dispatch<MouseScrolledEvent>([this](const MouseScrolledEvent& event) {
            OnMouseScroll(event);
            return true;
        });*/

        if (m_IsViewportHovered) {
            dispatcher.Dispatch<MouseButtonPressedEvent>([this](const MouseButtonPressedEvent& event) {
                switch (event.GetMouseButton()) {
                case MouseButton::Left:
                    m_LMB = true;
                    return true;
                case MouseButton::Right:
                    m_RMB = true;
                    Application::Get().GetWindow()->SetCursorMode(CursorMode::Locked);
                    return true;
                default:
                    break;
                }

                return false;
            });
        }

        if (m_LMB || m_RMB) {
            dispatcher.Dispatch<MouseButtonReleasedEvent>([this](const MouseButtonReleasedEvent& event) {
                switch (event.GetMouseButton()) {
                case MouseButton::Left:
                    m_LMB = false;
                    return true;
                case MouseButton::Right:
                    m_RMB = false;
                    Application::Get().GetWindow()->SetCursorMode(CursorMode::Normal);
                    return true;
                default:
                    break;
                }

                return false;
            });
        }

        dispatcher.Dispatch<MouseMovedEvent>([this](const MouseMovedEvent& event) {
            const float2 mouse(event.GetX(), -event.GetY());
            float2 delta = event.GetDelta() * m_MouseSensitivity;
            delta.y *= -1.0f;

            m_MouseDelta += delta;
            m_InitialMousePosition = mouse;

            return m_RMB;
        });

        if (m_LMB || m_RMB) {
            dispatcher.Dispatch<KeyPressedEvent>([this](const KeyPressedEvent& event) {
                switch (event.GetKeyCode()) {
                case KeyCode::LeftControl:
                    m_LControl = true;
                    break;
                case KeyCode::LeftShift:
                    m_LShift = true;
                    break;
                case KeyCode::W:
                    m_Forward = true;
                    break;
                case KeyCode::S:
                    m_Backwards = true;
                    break;
                case KeyCode::A:
                    m_Left = true;
                    break;
                case KeyCode::D:
                    m_Right = true;
                    break;
                default: break;
                }

                return false;
            });
        }

        dispatcher.Dispatch<KeyReleasedEvent>([this](const KeyReleasedEvent& event) {
            switch (event.GetKeyCode()) {
            case KeyCode::LeftControl:
                m_LControl = false;
                break;
            case KeyCode::LeftShift:
                m_LShift = false;
                break;
            case KeyCode::W:
                m_Forward = false;
                break;
            case KeyCode::S:
                m_Backwards = false;
                break;
            case KeyCode::A:
                m_Left = false;
                break;
            case KeyCode::D:
                m_Right = false;
                break;
            default: break;
            }

            return false;
        });
    }

    bool EditorCamera::OnMouseScroll(const MouseScrolledEvent& e) {
        const float delta = e.GetYOffset() * 0.1f;
        MouseZoom(delta);
        UpdateView();
        return false;
    }

    void EditorCamera::MousePan(const float2& delta) {
        const float2 speed = PanSpeed();
        m_FocalPoint += -GetRightDirection() * delta.x * speed.x * m_Distance;
        m_FocalPoint += GetUpDirection() * delta.y * speed.y * m_Distance;
    }

    void EditorCamera::MouseRotate(const float2& delta) {
        float yawSign = GetUpDirection().y < 0 ? -1.0f : 1.0f;
        m_Yaw += yawSign * delta.x * RotationSpeed();
        m_Pitch += delta.y * RotationSpeed();
    }

    void EditorCamera::MouseZoom(float delta) {
        m_Distance -= delta * ZoomSpeed();
        if (m_Distance < 1.0f) {
            m_FocalPoint += GetForwardDirection();
            m_Distance = 1.0f;
        }
    }

    void EditorCamera::PlayerMove(const float2& delta) {
        m_PlayerMode = true;
        m_Pitch += delta.y;
    }


    quaternion EditorCamera::GetOrientation() const {
        return quaternion::rotation_euler_zxy(float3(-m_Pitch, -m_Yaw, 0.0f));
    }

    float4x4 EditorCamera::GetViewProjection() const {
        return math::mul(m_ViewMatrix, m_Projection);
    }

    float3 EditorCamera::GetUpDirection() const {
        return math::mul(GetOrientation(), float3(0.0f, 1.0f, 0.0f));
    }

    float3 EditorCamera::GetRightDirection() const {
        return math::mul(GetOrientation(), float3(1.0f, 0.0f, 0.0f));
    }

    float3 EditorCamera::GetForwardDirection() const {
        return math::mul(GetOrientation(), float3(0.0f, 0.0f, -1.0f));
    }

    float3 EditorCamera::CalculatePosition() const {
        return m_FocalPoint - GetForwardDirection() * -m_Distance;
    }
}
