#pragma once

#include "imgui.h"
#include "Quelos/Scenes/Scene.h"

namespace Quelos {
    class EntityInspectorPanel {
    public:
        explicit EntityInspectorPanel(const Ref<Scene>& scene) : m_Scene(scene) {}

        void SetSelectedEntity(const Entity entity) { m_SelectedEntity = entity; }
        void ClearSelectedEntity() { m_SelectedEntity = {}; }

        void OnImGuiRender(ImGuiID dockspaceID, const ImGuiWindowClass& windowClass);
    private:
        Entity m_SelectedEntity;
        Ref<Scene> m_Scene;
    };
}
