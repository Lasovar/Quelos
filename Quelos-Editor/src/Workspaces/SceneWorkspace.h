#pragma once

#include "Workspace.h"
#include "imgui.h"

#include "Panels/ViewportPanel.h"
#include "Quelos/Scenes/Scene.h"

namespace Quelos {
    class SceneWorkspace : public Workspace {
    public:
        SceneWorkspace();

        void Tick(float deltaTime) override;
        void OnImGuiRender(unsigned int dockspaceID) override;

        void SetScene(const Ref<Scene>& scene);

    private:
        Ref<Scene> m_Scene;

        ViewportPanel m_ViewportPanel;
        ImGuiWindowClass m_SceneWorkspaceClass;
        std::string m_WorkspaceID;
    };
}
