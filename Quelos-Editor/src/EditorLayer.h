#pragma once

#include <Quelos/Core/Layer.h>
#include <Quelos/Scenes/Scene.h>
#include <Quelos/Core/Ref.h>

#include "UndoSystem.h"
#include "Panels/ViewportPanel.h"
#include "Workspaces/SceneWorkspace.h"

namespace Quelos {
	enum class SceneState {
		Edit = 0, Play
	};

	class EditorLayer : public Layer {
	public:
		EditorLayer();

		void OnAttach() override;

		void Tick(float deltaTime) override;
		void ImGuiRender() override;
		void OnScenePlay();
		void OnSceneStop();
		void UI_Toolbar();
		void OnEvent(Event& event) override;
	private:
		SceneState m_SceneState = SceneState::Edit;
		bool m_ScenePaused = false;
		bool m_SceneStep = false;

		Ref<Texture2D> m_IconPlay, m_IconPause, m_IconStop, m_IconStep, m_Container;

		Ref<Scene> m_DefaultScene;

		Ref<Scene> m_EditorScene;
		Ref<Scene> m_ActiveScene;

        ImGuiWindowClass m_EditorLayerClass;
		Ref<SceneWorkspace> m_SceneWorkspace;
		UndoSystem m_UndoSystem;

		std::vector<Ref<Workspace>> m_Workspaces;

		bool m_CtrlKey: 1 = false;
		bool m_ZKey: 1 = false;
		bool m_YKey: 1 = false;
		bool m_ShiftKey: 1 = false;
	};
}
