#pragma once

#include "Workspace.h"
#include "imgui.h"

#include "Panels/ViewportPanel.h"
#include "Quelos/Renderer/EditorCamera.h"
#include "Quelos/Scenes/Scene.h"

namespace Quelos {
    class SceneWorkspace : public Workspace {
    public:
        SceneWorkspace();

        void Tick(float deltaTime) override;
        void OnImGuiRender(unsigned int dockspaceID) override;

        void SetScene(const Ref<Scene>& scene);

        void OnEvent(Event& e);

    private:
        Ref<Scene> m_Scene;

        ViewportPanel m_GameViewportPanel;
        ViewportPanel m_SceneViewportPanel;
        EditorCamera m_EditorCamera;
        ImGuiWindowClass m_SceneWorkspaceClass;
        std::string m_WorkspaceID;
    };
}
