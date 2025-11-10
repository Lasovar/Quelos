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
		ImGui::ShowDemoWindow();
	}

}

