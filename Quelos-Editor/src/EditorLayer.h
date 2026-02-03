#pragma once

#include <Quelos/Core/Layer.h>
#include <Quelos/Scenes/Scene.h>
#include <Quelos/Core/Ref.h>

#include "Panels/ViewportPanel.h"
#include "Workspaces/SceneWorkspace.h"

namespace Quelos {
	class EditorLayer : public Layer {
	public:
		EditorLayer();

		void OnAttach() override;

		void Tick(float deltaTime) override;
		void ImGuiRender() override;
	private:
		Ref<Scene> m_DefaultScene;

		Ref<SceneWorkspace> m_SceneWorkspace;

		std::vector<Ref<Workspace>> m_Workspaces;
	};
}

