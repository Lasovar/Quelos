#include "qspch.h"
#include "Application.h"

#include <imgui.h>

#include "Quelos/Renderer/Renderer.h"
#include "Log.h"

namespace Quelos {
	Application* Application::s_Instance = nullptr;

	Application::Application(ApplicationSpecification appSpecs) {
		s_Instance = this;
		Log::Init();

		QS_CORE_INFO_TAG("Core", "Application {} initialized!", appSpecs.Name);

		m_Window = Window::Create(appSpecs.WindowSpec);
		m_Window->Init();

		Renderer::Init(m_Window);

		m_ImGuiLayer = PushLayer<ImGuiLayer>();
	}

	void Application::Run() {
		m_IsRunning = true;
		while (!m_Window->SholdClose()) {
			m_Window->Update();

			Renderer::StartFrame();

			for (auto& layer : m_LayerStack)
				layer->Tick(1.0f);

			m_ImGuiLayer->Begin();

			for (auto& layer : m_LayerStack)
				layer->ImGuiRender();

			m_ImGuiLayer->End();

			Renderer::EndFrame();
		}
	}

	void Application::Stop()
	{
		Renderer::Shutdown();
		Log::Shutdown();
	}
}

