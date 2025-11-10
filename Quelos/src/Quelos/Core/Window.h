#pragma once
#include <cstdint>
#include "Ref.h"

class GLFWwindow;

namespace Quelos {
	struct WindowSpecification
	{
		std::string Title;
		uint32_t Width = 0;
		uint32_t Height = 0;
	};

	class Window : public RefCounted {
	public:
		Window(const WindowSpecification& windowSpecs);

		void Init();

		void Update();

		uint32_t GetWidth() const { return m_Sepcifications.Width; }
		uint32_t GetHeight() const { return m_Sepcifications.Height; }

		void* GetWindowHandle() const { return m_GLFWWindow; }
		void* GetNativeWindow() const { return m_WindowHandle; }

		bool SholdClose() const;
	public:
		static Ref<Window> Create(const WindowSpecification& windowSpecification);
	private:
		WindowSpecification m_Sepcifications;
		GLFWwindow* m_GLFWWindow;
		void* m_WindowHandle = nullptr;
	};
}

