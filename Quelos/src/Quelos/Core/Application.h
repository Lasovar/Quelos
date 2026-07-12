#pragma once

#include <string>
#include <vector>

#include "Allocators.hpp"
#include "Base.h"
#include "Quelos/Project/Project.h"

#include "Quelos/Core/Timer.h"
#include "Quelos/Core/Window.h"
#include "Quelos/Core/Layer.h"

#include "Quelos/ImGui/ImGuiLayer.h"

#include "Quelos/Renderer/RendererAPI.h"

namespace Quelos {
	class ImGuiLayer;

	struct QS_API ApplicationSpecification {
		std::string Name;
		std::filesystem::path ApplicationPath;
		WindowSpecification WindowSpec;
		RendererAPI RendererAPI;
	};

	class QS_API Application {
	public:
		explicit Application(ApplicationSpecification appSpecs);

		void Run();
		void Stop();

		[[nodiscard]] Ref<Window> GetWindow() const { return m_Window; }
		[[nodiscard]] Ref<Time> GetTime() const { return m_Time;}
		[[nodiscard]] const OsPath& GetApplicationPath() const { return m_Specifications.ApplicationPath; }
		[[nodiscard]] const ApplicationSpecification& GetApplicationSpecification() const { return m_Specifications; }
		
		template <typename TLayer>
		requires(std::is_base_of_v<Layer, TLayer>)
		Ref<TLayer> PushLayer();

		void RaiseEvent(Event& event);

		void Tick() const;
	public:
		static Application& Get() { return *s_Instance; }

		static ArenaMemoryResource& GetTempAllocator() {
			return Get().m_TempAllocator;
		}

	private:
		static Application* s_Instance;
	private:
		Vec<Ref<Layer>> m_LayerStack{Allocator::Persistent};
		Ref<ImGuiLayer> m_ImGuiLayer;

		Ref<Time> m_Time;

		Ref<Window> m_Window;
		ApplicationSpecification m_Specifications;

		PagePool m_PagePool;
		LinearArena m_TempAllocatorArena;
		ArenaMemoryResource m_TempAllocator;

		bool m_IsRunning = false;
	};

	QS_API Application* CreateApplication(int argc, char** argv);

	template <typename TLayer>
		requires(std::is_base_of_v<Layer, TLayer>)
	Ref<TLayer> Application::PushLayer() {
		Ref<TLayer> newLayer = CreateRef<TLayer>();
		m_LayerStack.push_back(newLayer);
		newLayer->OnAttach();
		return newLayer;
	}
}
