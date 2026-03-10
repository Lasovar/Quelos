#pragma once

#include "Workspace.h"
#include "imgui.h"
#include "Panels/EntityInspectorPanel.h"

#include "Panels/ViewportPanel.h"
#include "Quelos/Renderer/EditorCamera.h"
#include "Quelos/Scenes/Scene.h"

namespace Quelos {
    class SceneWorkspace : public Workspace {
    public:
        explicit SceneWorkspace(const Ref<Scene>& scene, UndoSystem& undoSystem);

        void SelectEntity(Entity entity);

        void Tick(float deltaTime) override;
        void OnImGuiRender(ImGuiID dockspaceID) override;

        void OnEvent(Event& e);

    private:
        Ref<Scene> m_Scene;
        UndoSystem& m_UndoSystem;

        ViewportPanel m_GameViewportPanel;
        ViewportPanel m_SceneViewportPanel;

        EntityInspectorPanel m_InspectorPanel;

        EditorCamera m_EditorCamera;
        ImGuiWindowClass m_SceneWorkspaceClass;
        std::string m_WorkspaceID;
    };
}
