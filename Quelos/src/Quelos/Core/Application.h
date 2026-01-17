#pragma once

#include <string>
#include <vector>

#include "Base.h"
#include "Window.h"
#include "Layer.h"
#include "Quelos/ImGUI/ImGuiLayer.h"

namespace Quelos {
	class ImGuiLayer;

	struct ApplicationSpecification {
		std::string Name;
		std::string Executable;
		WindowSpecification WindowSpec;
	};

	class Application {
	public:
		Application(ApplicationSpecification appSpecs);

		void Run();
		void Stop();

		Ref<Window> GetWindow() { return m_Window; }
		ApplicationSpecification GetApplicationSpecification() const { return m_Specifications; }
		
		template <typename TLayer>
		requires(std::is_base_of_v<Layer, TLayer>)
		std::shared_ptr<TLayer> PushLayer();

	public:
		static Application& Get() { return *s_Instance; }

	private:
		static Application* s_Instance;
	private:
		std::vector<std::shared_ptr<Layer>> m_LayerStack;
		std::shared_ptr<ImGuiLayer> m_ImGuiLayer;

		Ref<Window> m_Window;
		ApplicationSpecification m_Specifications;
		bool m_IsRunning;
	};

	Application* CreateApplication(int argc, char** argv);

	template <typename TLayer>
		requires(std::is_base_of_v<Layer, TLayer>)
	std::shared_ptr<TLayer> Application::PushLayer() {
		std::shared_ptr<TLayer> newLayer = std::make_shared<TLayer>();
		m_LayerStack.push_back(newLayer);
		newLayer->OnAttach();
		return newLayer;
	}
}

