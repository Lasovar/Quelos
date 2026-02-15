#pragma once

#include "Quelos/Core/Event.h"
#include "Quelos/Core/Events/InputEvents.h"
#include "Quelos/Renderer/Camera.h"

namespace Quelos {
	class EditorCamera : public Camera
	{
	public:
		EditorCamera() = default;
		EditorCamera(float fov, float aspectRatio, float nearClip, float farClip);

		void OnUpdate(float deltaTime);
		void OnEvent(Event& e);

		[[nodiscard]] float GetDistance() const { return m_Distance; }
		void SetDistance(const float distance) { m_Distance = distance; }

		void SetViewportSize(const float width, const float height) {
			m_ViewportWidth = width;
			m_ViewportHeight = height;
			UpdateProjection();
		}

		bool IsLockRotation() const { return m_LockRotation; }
		void SetLockRotation(const bool value) { m_LockRotation = value; }

		[[nodiscard]] const glm::mat4& GetViewMatrix() const { return m_ViewMatrix; }
		[[nodiscard]] glm::mat4 GetViewProjection() const;

		[[nodiscard]] glm::vec3 GetUpDirection() const;
		[[nodiscard]] glm::vec3 GetRightDirection() const;
		[[nodiscard]] glm::vec3 GetForwardDirection() const;
		[[nodiscard]] const glm::vec3& GetPosition() const { return m_Position; }
		[[nodiscard]] glm::quat GetOrientation() const;

		[[nodiscard]] float GetPitch() const { return m_Pitch; }
		[[nodiscard]] float GetYaw() const { return m_Yaw; }
	private:
		void UpdateProjection();
		void UpdateView();

		bool OnMouseScroll(const MouseScrolledEvent& e);

		void MousePan(const glm::vec2& delta);
		void MouseRotate(const glm::vec2& delta);
		void MouseZoom(float delta);
		void PlayerMove(const glm::vec2& delta);

		[[nodiscard]] glm::vec3 CalculatePosition() const;

		[[nodiscard]] glm::vec2 PanSpeed() const;
		[[nodiscard]] float ZoomSpeed() const;
		[[nodiscard]] static float RotationSpeed();
	private:
		float m_FOV = 60.0f, m_AspectRatio = 1.778f, m_NearClip = 0.1f, m_FarClip = 1000.0f;

		glm::mat4 m_ViewMatrix{};
		glm::vec3 m_Position = { 0.0f, 0.0f, -15.0f };
		glm::vec3 m_FocalPoint = { 0.0f, 0.0f, 0.0f };

		float m_MoveSpeed = 100.0f;

		glm::vec3 m_Velocity = { 0.0f, 0.0f, 0.0f };

		float m_Acceleration = 40.0f;
		float m_Damping = 8.0f;

		float m_MouseSensitivity = 0.05f;

		glm::vec2 m_MouseDelta = { 0.0f, 0.0f };
		glm::vec2 m_InitialMousePosition = { 0.0f, 0.0f };

		float m_Distance = 10.0f;
		float m_Pitch = 0.0f, m_Yaw = 0.0f;

		float m_ViewportWidth = 1280, m_ViewportHeight = 720;
		bool m_PlayerMode = false, m_LockRotation = false;

		// Input
		bool m_LControl: 1 = false;
		bool m_LShift: 1 = false;
		bool m_RMB: 1 = false, m_LMB: 1 = false;

		bool m_Up: 1{};
		bool m_Down: 1{};
		bool m_Forward: 1{};
		bool m_Backwards: 1{};
		bool m_Left: 1{};
		bool m_Right: 1{};
	};
}
