#include "qspch.h"
#include "EditorCamera.h"

#include "Quelos/Core/Events/InputEvents.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

#include "bgfx/bgfx.h"
#include "bx/math.h"
#include "glm/gtc/type_ptr.hpp"
#include "Quelos/Math/Math.h"

namespace Quelos {
    EditorCamera::EditorCamera(const float fov, const float aspectRatio, const float nearClip, const float farClip)
        : Camera(glm::perspective(glm::radians(fov), aspectRatio, nearClip, farClip)),
          m_FOV(fov), m_AspectRatio(aspectRatio), m_NearClip(nearClip), m_FarClip(farClip) {
        UpdateView();
        MousePan(glm::vec2{0.01f});
    }

    void EditorCamera::UpdateProjection() {
        m_AspectRatio = m_ViewportWidth / m_ViewportHeight;

        bx::mtxProj(
            glm::value_ptr(m_Projection),
            m_FOV,
            m_AspectRatio,
            m_NearClip,
            m_FarClip,
            bgfx::getCaps()->homogeneousDepth
        );
    }

    void EditorCamera::UpdateView() {
        if (m_LockRotation) {
            m_Yaw = m_Pitch = 0.0f; // Lock the camera's rotation
        }

        const glm::quat qPitch = glm::angleAxis(m_Pitch, glm::vec3(1,0,0));
        const glm::quat qYaw   = glm::angleAxis(m_Yaw,   glm::vec3(0,1,0));

        const glm::quat orientation = qYaw * qPitch;

        m_ViewMatrix = Math::ViewMatrix(orientation, m_Position);
        QS_CORE_INFO("{}, {}, {}", m_Position.x, m_Position.y, m_Position.z);
    }

    glm::vec2 EditorCamera::PanSpeed() const {
        const float x = std::min(m_ViewportWidth / 1000.0f, 2.4f); // max = 2.4f
        float xFactor = 0.0366f * (x * x) - 0.1778f * x + 0.3021f;

        const float y = std::min(m_ViewportHeight / 1000.0f, 2.4f); // max = 2.4f
        float yFactor = 0.0366f * (y * y) - 0.1778f * y + 0.3021f;

        return {xFactor, yFactor};
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
        glm::vec2 smoothed = m_MouseDelta;
        m_MouseDelta = glm::vec2(0.0f);

        if (!m_RMB) {
            return;
        }

        m_Yaw   -= smoothed.x * m_MouseSensitivity;
        m_Pitch -= smoothed.y * m_MouseSensitivity;

        m_Pitch = glm::clamp(m_Pitch, -glm::radians(89.0f), glm::radians(89.0f));

        // Direction vectors
        glm::vec3 forwardDir{
            glm::cos(m_Pitch) * glm::sin(m_Yaw),
            glm::sin(m_Pitch),
           -glm::cos(m_Pitch) * glm::cos(m_Yaw)
        };

        glm::vec3 rightDir = glm::normalize(glm::cross(forwardDir, glm::vec3(0,1,0)));
        glm::vec3 upDir    = glm::cross(rightDir, forwardDir);

        // Desired movement
        glm::vec3 input{0.0f};

        if (m_Forward)      input -= forwardDir;
        if (m_Backwards)    input += forwardDir;
        if (m_Right)        input += rightDir;
        if (m_Left)         input -= rightDir;
        if (m_Up)           input += upDir;
        if (m_Down)         input -= upDir;

        if (glm::length(input) > 0.0f) {
            input = glm::normalize(input);
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

        dispatcher.Dispatch<MouseButtonPressedEvent>([this](const MouseButtonPressedEvent& event) {
            switch (event.GetMouseButton()) {
            case MouseButton::Left:
                m_LMB = true;
                break;
            case MouseButton::Right:
                m_RMB = true;
                break;
            default:
                break;
            }

            return false;
        });

        dispatcher.Dispatch<MouseButtonReleasedEvent>([this](const MouseButtonReleasedEvent& event) {
            switch (event.GetMouseButton()) {
            case MouseButton::Left:
                m_LMB = false;
                break;
            case MouseButton::Right:
                m_RMB = false;
                break;
            default:
                break;
            }

            return false;
        });

        dispatcher.Dispatch<MouseMovedEvent>([this](const MouseMovedEvent& event) {
            const glm::vec2 mouse(event.GetX(), -event.GetY());
            glm::vec2 delta = event.GetDelta() * m_MouseSensitivity;
            delta.y *= -1.0f;

            m_MouseDelta += delta;
            m_InitialMousePosition = mouse;

            return false;
        });

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

            return true;
        });

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

            return true;
        });
    }

    bool EditorCamera::OnMouseScroll(const MouseScrolledEvent& e) {
        const float delta = e.GetYOffset() * 0.1f;
        MouseZoom(delta);
        UpdateView();
        return false;
    }

    void EditorCamera::MousePan(const glm::vec2& delta) {
        const glm::vec2 speed = PanSpeed();
        m_FocalPoint += -GetRightDirection() * delta.x * speed.x * m_Distance;
        m_FocalPoint += GetUpDirection() * delta.y * speed.y * m_Distance;
    }

    void EditorCamera::MouseRotate(const glm::vec2& delta) {
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

    void EditorCamera::PlayerMove(const glm::vec2& delta) {
        m_PlayerMode = true;
        m_Pitch += delta.y;
    }

    glm::mat4 EditorCamera::GetViewProjection() const {
        glm::mat4 result;
        bx::mtxMul(glm::value_ptr(result), glm::value_ptr(m_ViewMatrix), glm::value_ptr(m_Projection));
        return result;
    }

    glm::vec3 EditorCamera::GetUpDirection() const {
        return glm::rotate(GetOrientation(), glm::vec3(0.0f, 1.0f, 0.0f));
    }

    glm::vec3 EditorCamera::GetRightDirection() const {
        return glm::rotate(GetOrientation(), glm::vec3(1.0f, 0.0f, 0.0f));
    }

    glm::vec3 EditorCamera::GetForwardDirection() const {
        return glm::rotate(GetOrientation(), glm::vec3(0.0f, 0.0f, -1.0f));
    }

    glm::vec3 EditorCamera::CalculatePosition() const {
        return m_FocalPoint - GetForwardDirection() * -m_Distance;
    }

    glm::quat EditorCamera::GetOrientation() const {
        return glm::quat(glm::vec3(-m_Pitch, -m_Yaw, 0.0f));
    }
}
