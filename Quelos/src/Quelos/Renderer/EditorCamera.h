#pragma once

#include "Quelos/Core/Event.h"
#include "Quelos/Core/Events/InputEvents.h"
#include "Quelos/Renderer/Camera.h"

namespace Quelos {
	class QS_API EditorCamera : public Camera
	{
	public:
		EditorCamera() = default;
		EditorCamera(float fov, float aspectRatio, float nearClip, float farClip);

		void OnUpdate(float deltaTime);
		void OnEvent(Event& e);

		void ClearInput() {
			m_Up = false;
			m_Down = false;
			m_Forward = false;
			m_Backwards = false;
			m_Left = false;
			m_Right = false;
			m_LMB = false;
			m_RMB = false;
		}

		[[nodiscard]] float GetDistance() const { return m_Distance; }
		void SetDistance(const float distance) { m_Distance = distance; }

		void SetViewportSize(const float width, const float height) {
			m_ViewportWidth = width;
			m_ViewportHeight = height;
			UpdateProjection();
		}

		void SetViewportHovered(const bool value) { m_IsViewportHovered = value; }
		void SetViewportFocused(const bool value) { m_IsViewportFocused = value; }

		[[nodiscard]] bool IsLockRotation() const { return m_LockRotation; }
		void SetLockRotation(const bool value) { m_LockRotation = value; }

		[[nodiscard]] const float4x4& GetViewMatrix() const { return m_ViewMatrix; }
		[[nodiscard]] float4x4 GetViewProjection() const;

		[[nodiscard]] float3 GetUpDirection() const;
		[[nodiscard]] float3 GetRightDirection() const;
		[[nodiscard]] float3 GetForwardDirection() const;
		[[nodiscard]] float3 GetPosition() const { return m_Position; }
		[[nodiscard]] quaternion GetOrientation() const;

		[[nodiscard]] float GetPitch() const { return m_Pitch; }
		[[nodiscard]] float GetYaw() const { return m_Yaw; }
	private:
		void UpdateProjection();
		void UpdateView();

		bool OnMouseScroll(const MouseScrolledEvent& e);

		void MousePan(const float2& delta);
		void MouseRotate(const float2& delta);
		void MouseZoom(float delta);
		void PlayerMove(const float2& delta);

		[[nodiscard]] float3 CalculatePosition() const;

		[[nodiscard]] float2 PanSpeed() const;
		[[nodiscard]] float ZoomSpeed() const;
		[[nodiscard]] static float RotationSpeed();
	private:
		float m_FOV = 60.0f, m_AspectRatio = 1.778f, m_NearClip = 0.1f, m_FarClip = 1000.0f;

		float4x4 m_ViewMatrix{};
		float3 m_Position = { 0.0f, 0.0f, -15.0f };
		float3 m_FocalPoint = { 0.0f, 0.0f, 0.0f };

		float m_MoveSpeed = 100.0f;

		float3 m_Velocity = { 0.0f, 0.0f, 0.0f };

		float m_Acceleration = 40.0f;
		float m_Damping = 8.0f;

		float m_MouseSensitivity = 0.05f;

		float2 m_MouseDelta = { 0.0f, 0.0f };
		float2 m_InitialMousePosition = { 0.0f, 0.0f };

		float m_Distance = 10.0f;
		float m_Pitch = 0.0f, m_Yaw = 0.0f;

		float m_ViewportWidth = 1280, m_ViewportHeight = 720;
		bool m_IsViewportHovered:1 = false, m_IsViewportFocused:1 = false;
		bool m_PlayerMode:1 = false, m_LockRotation:1 = false;

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
