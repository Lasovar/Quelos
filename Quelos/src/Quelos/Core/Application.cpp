#include "qspch.h"
#include "Application.h"

#include "Base.h"
#include "Quelos/Core/Log.h"
#include "Quelos/Core/Events/WindowEvents.h"

#include "Quelos/Renderer/Renderer.h"
#include "flecs.h"
#include "Quelos/Core/Profiling.h"

namespace Quelos {

	QS_API Application* Application::s_Instance = nullptr;

	Application::Application(ApplicationSpecification appSpecs)
		: m_Specifications(appSpecs)
	{
		s_Instance = this;
		Log::Init(m_Specifications.Name);

		m_Time = CreateRef<Time>();
		m_Time->Init();

		QS_CORE_INFO_TAG("Core", "Application {} initialized!", appSpecs.Name);

		m_Window = Window::Create(appSpecs.WindowSpec);
		m_Window->Init();

		Renderer::Init(m_Window, appSpecs.RendererAPI);

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
			m_Window->PollEvents();
			Tick();

			QS_PROFILE_FRAME();
		}
	}

	void Application::Tick() const {
		Renderer::StartFrame();

		{
			QS_PROFILE_SCOPED_N("LayersTick");
			for (auto& layer : m_LayerStack) {
				layer->Tick(m_Time->DeltaTime());
			}
		}

		{
			QS_PROFILE_SCOPED_N("ImGuiRender");
			m_ImGuiLayer->Begin();

			for (auto& layer : m_LayerStack) {
				layer->ImGuiRender();
			}

			static bool showStats = true;

			static float timer = 0.0f;
			static int frameCount = 0;
			static float displayedFPS = 0.0f;

			timer += m_Time->DeltaTime();
			frameCount++;

			if (timer >= 0.25f) {
				displayedFPS = frameCount / timer;
				frameCount = 0;
				timer = 0.0f;
			}

			if (ImGui::Begin("Stats", &showStats)) {
				ImGui::Text("Frame time: %.3f ms", m_Time->DeltaTime() * 1000.f);
				ImGui::Text("Frame rate: %.3f fps", displayedFPS);
			}
			ImGui::End();

			m_ImGuiLayer->End();
		}

		Renderer::EndFrame();

		m_Time->Tick();
	}

	void Application::Stop() {
		m_LayerStack.clear();

		Renderer::Shutdown();
		Log::Shutdown();
	}
}
