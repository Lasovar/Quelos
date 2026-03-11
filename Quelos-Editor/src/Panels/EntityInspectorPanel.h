#pragma once

#include "imgui.h"
#include "InspectorArchive.h"
#include "Quelos/Scenes/Scene.h"

#include "UndoSystem.h"

namespace Quelos {
    using InspectorArchiveSerializeFn = void(*)(InspectorArchive&, void*);

    struct InspectorComponent {
        InspectorArchiveSerializeFn InspectorSerializeFn = nullptr;
        SetFieldSerializeFn SetFieldSerializeFn = nullptr;
        std::string_view ComponentName;
    };

    class EntityInspectorPanel {
    public:
        explicit EntityInspectorPanel(const Ref<Scene>& scene, UndoSystem& undoSystem);

        void SetSelectedEntity(const Entity entity) { m_SelectedEntity = entity; }
        void ClearSelectedEntity() { m_SelectedEntity = {}; }

        void OnImGuiRender(ImGuiID dockspaceID, const ImGuiWindowClass& windowClass);

    private:
        Map<RuntimeID, InspectorComponent> m_InspectorArchiveSerialize;
        Entity m_SelectedEntity;
        Ref<Scene> m_Scene;
        UndoSystem& m_UndoSystem;
    };
}
