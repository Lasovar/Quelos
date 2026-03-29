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
        HashMap<Serialization::PathID, std::string> FormattedFieldNames;
    };

    class EntityInspectorPanel {
    public:
        explicit EntityInspectorPanel(const Ref<Scene>& scene, UndoSystem& undoSystem);

        void SetSelectedEntity(const Actor& entity) {
            m_SelectedActor = entity;
            std::snprintf(m_EntityNameField.data(), m_EntityNameField.size(), "%s", entity.GetName());
        }

        void ClearSelectedEntity() { m_SelectedActor = {}; }

        bool ComponentHeader(const char* label, RuntimeID runtimeId, bool& open);
        void OnImGuiRender(ImGuiID dockspaceID, const ImGuiWindowClass& windowClass);

    private:
        static HashMap<RuntimeID, InspectorComponent> s_InspectorArchiveSerialize;

    private:
        std::array<char, 64> m_EntityNameField;

        Actor m_SelectedActor;
        Ref<Scene> m_Scene;
        UndoSystem& m_UndoSystem;
    };
}
