#pragma once

#include "Quelos/Core/Base.h"
#include "InspectorArchive.h"
#include "Quelos/Scenes/Scene.h"

#include "UndoSystem.h"

namespace QuelosEditor {
    using namespace Quelos;

    using InspectorArchiveSerializeFn = void(*)(InspectorArchive&, void*);

    struct InspectorComponent {
        InspectorArchiveSerializeFn InspectorSerializeFn = nullptr;
        SetFieldSerializeFn SetFieldSerializeFn = nullptr;
        std::string ComponentName;
        HashMap<Serialization::PathID, std::string> FormattedFieldNames;
    };

    using DrawComponentInspector = void(*)(void*);

    struct CustomInspector {
        std::string ComponentName;
        ComponentID ComponentId;
        DrawComponentInspector DrawFn = nullptr;
    };

    class EntityInspectorPanel {
    public:
        explicit EntityInspectorPanel(const Ref<Scene>& scene, UndoSystem& undoSystem);

        void SetSelectedEntity(const Actor& entity) {
            m_SelectedActor = entity;
            std::snprintf(m_EntityNameField.data(), m_EntityNameField.size(), "%s", entity.GetName());
        }

        void RegisterCustomInspector(const CustomInspector& customInspector) {
            m_CustomInspectors[m_Scene->GetComponentRegistry().GetComponentInfo(customInspector.ComponentId)->RuntimeID]
                = customInspector;
        }

        void ClearSelectedEntity() { m_SelectedActor = {}; }

        bool ComponentHeader(const char* label, RuntimeID runtimeId);
        void OnImGuiRender(ImGuiID dockspaceID, const ImGuiWindowClass& windowClass);

    private:
        static HashMap<RuntimeID, InspectorComponent> s_InspectorArchiveSerialize;

    private:
        std::array<char, 64> m_EntityNameField{};
        HashMap<Entity, HashSet<RuntimeID>> m_CollapsedComponents;
        HashMap<RuntimeID, CustomInspector> m_CustomInspectors;

        Actor m_SelectedActor;
        Ref<Scene> m_Scene;
        UndoSystem& m_UndoSystem;
    };
}
