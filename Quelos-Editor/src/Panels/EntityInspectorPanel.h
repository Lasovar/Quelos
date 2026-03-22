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
        std::string ComponentName;
    };

    class EntityInspectorPanel {
    public:
        explicit EntityInspectorPanel(const Ref<Scene>& scene, UndoSystem& undoSystem);

        void SetSelectedEntity(const Actor& entity) { m_SelectedActor = entity; }
        void ClearSelectedEntity() { m_SelectedActor = {}; }

        void OnImGuiRender(ImGuiID dockspaceID, const ImGuiWindowClass& windowClass);

    private:
        HashMap<RuntimeID, InspectorComponent> m_InspectorArchiveSerialize;
        Actor m_SelectedActor;
        Ref<Scene> m_Scene;
        UndoSystem& m_UndoSystem;
    };
}
