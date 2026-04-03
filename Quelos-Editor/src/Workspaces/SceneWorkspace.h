#pragma once

#include "Workspace.h"
#include "imgui.h"
#include "Panels/ContentBrowserPanel.h"
#include "Panels/EntityHierarchyPanel.h"
#include "Panels/EntityInspectorPanel.h"

#include "Panels/ViewportPanel.h"
#include "Quelos/Renderer/EditorCamera.h"
#include "Quelos/Scenes/Scene.h"

namespace QuelosEditor {
    using namespace Quelos;

    class SceneWorkspace : public Workspace {
    public:
        explicit SceneWorkspace(const Ref<Scene>& scene, UndoSystem& undoSystem);

        void SelectEntity(Entity entity);

        void Tick(float deltaTime) override;
        void WorkspaceContents() override;

        void OnEvent(Event& event) override;

    private:
        Ref<Scene> m_Scene;
        UndoSystem& m_UndoSystem;
        SceneSerializer m_SceneSerializer;

        ViewportPanel m_GameViewportPanel;
        ViewportPanel m_SceneViewportPanel;

        EntityInspectorPanel m_InspectorPanel;
        EntityHierarchyPanel m_EntityHierarchyPanel;

        ContentBrowserPanel m_ContentBrowserPanel;

        EditorCamera m_EditorCamera;

        bool m_CtrlKey: 1 = false;
        bool m_ShiftKey: 1 = false;
    };
}
