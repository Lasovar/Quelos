#include "EditorLayer.h"
#include <imgui.h>

#include <Quelos/Scenes/Components.h>
#include <Quelos/Core/Log.h>

namespace Quelos {

	static Entity s_Player;

	EditorLayer::EditorLayer() {
		s_Player = m_DefaultScene.CreateEntity("Player");

		s_Player.Add<TransformComponent>();
	}

	void EditorLayer::OnAttach() {
		
	}

	void EditorLayer::Tick(float deltaTime) {
		m_DefaultScene.Tick(deltaTime);
	}

	void EditorLayer::ImGuiRender() {
		static bool open = true;

		static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;

        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

		ImGui::Begin("MainDockSpace", &open, window_flags);

        ImGui::PopStyleVar(3);

		ImGuiIO& io = ImGui::GetIO();
		if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
			ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
			ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
		}

		ImGui::End();

		ImGui::Begin("Trial Main Space");
		ImGui::End();

		ImGui::ShowDemoWindow();
		//ImGui::ShowExampleDockSpace();	
	}

}

