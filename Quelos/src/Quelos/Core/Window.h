#pragma once
#include "Ref.h"

namespace Quelos {
	enum class QS_API WindowingBackend {
		None,
		SDL,
		GLFW,
	};

	struct QS_API WindowSpecification {
		std::string Title;
		uint32_t Width = 0;
		uint32_t Height = 0;
		WindowingBackend Backed = WindowingBackend::SDL;
	};

	enum class QS_API CursorMode : uint8_t {
		Normal,
		Hidden,
		Locked
	};

	class QS_API Window : public RefCounted<Window> {
	public:
		virtual ~Window() = default;

		virtual void Init() = 0;
		virtual void Shutdown() = 0;

		virtual void PollEvents() = 0;

		virtual void SetCursorMode(CursorMode cursorMode) = 0;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;

		virtual WindowingBackend GetWindowBacked() const = 0;

		virtual bool IsWayland() const = 0;

		virtual void* GetWindowHandle() const = 0;
		virtual void* GetNativeWindow() const = 0;
		virtual void* GetNativeDisplay() const = 0;

		virtual bool ShouldClose() const = 0;
	public:
		static Ref<Window> Create(const WindowSpecification& windowSpecification);
	protected:
		static void OnResize(uint32_t width, uint32_t height);
	};
}

