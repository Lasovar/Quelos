#include "qspch.h"
#include "Application.h"

#include <imgui.h>

#include "Quelos/AssetManager/AssetManagerBase.h"
#include "Quelos/Renderer/Renderer.h"
#include "Log.h"
#include "Events/WindowEvents.h"

namespace Quelos {
	Application* Application::s_Instance = nullptr;

	Application::Application(ApplicationSpecification appSpecs)
		: m_Specifications(appSpecs)
	{
		s_Instance = this;
		Log::Init();

		m_Time = CreateRef<Time>();
		m_Time->Init();

		QS_CORE_INFO_TAG("Core", "Application {} initialized!", appSpecs.Name);

		m_Window = Window::Create(appSpecs.WindowSpec);
		m_Window->Init();

		Renderer::Init(m_Window);

		m_ImGuiLayer = PushLayer<ImGuiLayer>();
	}

	void Application::RaiseEvent(Event& event) {
		Renderer::OnEvent(event);

		for (const auto& layer : std::views::reverse(m_LayerStack)) {
			layer->OnEvent(event);
			if (event.Handled) {
				break;
			}
		}
	}

	void Application::Run() {
		m_IsRunning = true;
		while (!m_Window->ShouldClose()) {
			m_Window->Update();

			Renderer::StartFrame();

			for (auto& layer : m_LayerStack) {
				layer->Tick(m_Time->DeltaTime());
			}

			m_ImGuiLayer->Begin();

			for (auto& layer : m_LayerStack) {
				layer->ImGuiRender();
			}

			m_ImGuiLayer->End();

			Renderer::EndFrame();

			m_Time->Tick();
		}
	}

	void Application::Stop() {
		Renderer::Shutdown();
		Log::Shutdown();
	}
}

