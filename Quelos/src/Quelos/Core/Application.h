#pragma once

#include <string>
#include <vector>

#include "Quelos/Core/Timer.h"
#include "Quelos/Core/Window.h"
#include "Quelos/Core/Layer.h"

#include "Quelos/ImGUI/ImGuiLayer.h"

#include "Quelos/AssetManager/AssetManagerBase.h"
#include "Quelos/AssetManager/EditorAssetManager.h"
#include "Quelos/AssetManager/RuntimeAssetManager.h"
#include "Quelos/Renderer/RendererAPI.h"

namespace Quelos {
	class ImGuiLayer;

	struct ApplicationSpecification {
		std::string Name;
		std::string Executable;
		WindowSpecification WindowSpec;
		RendererAPI RendererAPI;
	};

	class Application {
	public:
		explicit Application(ApplicationSpecification appSpecs);

		void Run();
		void Stop();

		[[nodiscard]] Ref<Window> GetWindow() const { return m_Window; }
		[[nodiscard]] Ref<Time> GetTime() const { return m_Time;}
		[[nodiscard]] Ref<AssetManagerBase> GetAssetManager() const { return m_AssetManager; }
		[[nodiscard]] Ref<RuntimeAssetManager> GetRuntimeAssetManager() const { return RefAs<RuntimeAssetManager>(m_AssetManager); }
		[[nodiscard]] Ref<EditorAssetManager> GetEditorAssetManager() const { return RefAs<EditorAssetManager>(m_AssetManager); }
		[[nodiscard]] ApplicationSpecification GetApplicationSpecification() const { return m_Specifications; }
		
		template <typename TLayer>
		requires(std::is_base_of_v<Layer, TLayer>)
		Ref<TLayer> PushLayer();

		void RaiseEvent(Event& event);

		void Tick() const;
	public:
		static Application& Get() { return *s_Instance; }
	private:
		static Application* s_Instance;
	private:
		std::vector<Ref<Layer>> m_LayerStack;
		Ref<ImGuiLayer> m_ImGuiLayer;

		Ref<Time> m_Time;
		Ref<AssetManagerBase> m_AssetManager;

		Ref<Window> m_Window;
		ApplicationSpecification m_Specifications;
		bool m_IsRunning{};
	};

	Application* CreateApplication(int argc, char** argv);

	template <typename TLayer>
		requires(std::is_base_of_v<Layer, TLayer>)
	Ref<TLayer> Application::PushLayer() {
		Ref<TLayer> newLayer = CreateRef<TLayer>();
		m_LayerStack.push_back(newLayer);
		newLayer->OnAttach();
		return newLayer;
	}
}
