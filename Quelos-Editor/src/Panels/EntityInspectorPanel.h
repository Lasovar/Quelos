#pragma once

#include "Quelos/Core/Base.h"
#include "InspectorArchive.h"
#include "Quelos/Scenes/Scene.h"

#include "UndoSystem.h"

namespace QuelosEditor {
    class SceneWorkspace;
    using namespace Quelos;

    using InspectorArchiveSerializeFn = void(*)(InspectorArchive&, void*);

    struct InspectorComponent {
        InspectorArchiveSerializeFn InspectorSerializeFn = nullptr;
        SetFieldSerializeFn SetFieldSerializeFn = nullptr;
        std::string ComponentName;
        HashMap<Serialization::PathID, std::string> FormattedFieldNames;
    };

    using DrawComponentInspector = std::function<void(void* componentData, const InspectorComponent& componentInfo, Entity entity)>;

    struct CustomInspector {
        std::string ComponentName;
        ComponentID ComponentId;
        DrawComponentInspector DrawFn = nullptr;
    };

    class EntityInspectorPanel {
    public:
        explicit EntityInspectorPanel(SceneWorkspace& sceneWorkspace, UndoSystem& undoSystem);

        void SetInspectorEntityName(const Entity& entity);

        void SetScene(const Ref<Scene>& scene);

        void RegisterCustomInspector(const CustomInspector& customInspector) {
            m_CustomInspectors[customInspector.ComponentId] = customInspector;
        }

        void ClearSelectedEntity() const;

        bool ComponentHeader(const char* label, ComponentID componentId);
        void OnImGuiRender(ImGuiID dockspaceID, const ImGuiWindowClass& windowClass);

    private:
        static HashMap<ComponentID, InspectorComponent> s_InspectorArchiveSerialize;

    private:
        std::array<char, 64> m_EntityNameField{};
        HashMap<Entity, HashSet<ComponentID>> m_CollapsedComponents;
        HashMap<ComponentID, CustomInspector> m_CustomInspectors;

        Ref<Scene> m_Scene;
        SceneWorkspace& m_SceneWorkspace;
        UndoSystem& m_UndoSystem;
    };
}
