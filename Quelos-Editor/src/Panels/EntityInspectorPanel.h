#pragma once

#include "imgui.h"
#include "Quelos/Scenes/Scene.h"

#include "UndoSystem.h"

namespace Quelos {
    class EntityInspectorPanel {
    public:
        explicit EntityInspectorPanel(const Ref<Scene>& scene, UndoSystem& undoSystem)
            : m_Scene(scene), m_UndoSystem(undoSystem) { }

        void SetSelectedEntity(const Entity entity) { m_SelectedEntity = entity; }
        void ClearSelectedEntity() { m_SelectedEntity = {}; }

        void OnImGuiRender(ImGuiID dockspaceID, const ImGuiWindowClass& windowClass);

    private:
        Entity m_SelectedEntity;
        Ref<Scene> m_Scene;
        UndoSystem& m_UndoSystem;
    };
}
