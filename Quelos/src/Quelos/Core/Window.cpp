#include "qspch.h"
#include "Window.h"

#define GLFW_EXPOSE_NATIVE_WIN32

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <iostream>

namespace Quelos {
	static bool s_GLFWInitialized = false;

	static void GLFWErrorCallback(int error, const char* description) {
		QS_CORE_ERROR_TAG("GLFW", "GLFW Error ({0}): {1}", error, description);
	}

	Window::Window(const WindowSpecification& windowSpecs)
		: m_Sepcifications(windowSpecs), m_WindowHandle(nullptr), m_GLFWWindow(nullptr)
	{
	}

	void Window::Init() {
		if (!s_GLFWInitialized) {
			int success = glfwInit();
			QS_CORE_ASSERT(success, "Could not initialize GLFW");
			glfwSetErrorCallback(GLFWErrorCallback);

			s_GLFWInitialized = true;
		}

		m_Sepcifications.Width = m_Sepcifications.Width;
		m_Sepcifications.Height = m_Sepcifications.Height;

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		m_GLFWWindow = glfwCreateWindow(
			m_Sepcifications.Width,
			m_Sepcifications.Height, 
			m_Sepcifications.Title.c_str(),
			nullptr, 
			nullptr
		);

		m_WindowHandle = glfwGetWin32Window(m_GLFWWindow);
	}

	void Window::Update()
	{
		glfwPollEvents();
	}

	Ref<Window> Window::Create(const WindowSpecification& windowSpecification)
	{
		return Ref<Window>::Create(windowSpecification);
	}

	bool Window::SholdClose() const
	{
		return glfwWindowShouldClose(m_GLFWWindow);
	}

}

