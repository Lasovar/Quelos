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
        HashMap<Serialization::PathID, std::string> FormattedFieldNames{Allocator::Persistent};

        InspectorComponent(InspectorComponent&&) noexcept = default;
        InspectorComponent& operator=(InspectorComponent&&) noexcept = default;

        InspectorComponent(
            InspectorArchiveSerializeFn serialize,
            Quelos::SetFieldSerializeFn setField,
            std::string name,
            HashMap<Serialization::PathID, std::string> fields
        )
            : InspectorSerializeFn(serialize)
              , SetFieldSerializeFn(setField)
              , ComponentName(std::move(name))
              , FormattedFieldNames(std::move(fields)) {}

        InspectorComponent(const InspectorComponent&) = delete;
        InspectorComponent& operator=(const InspectorComponent&) = delete;
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

        void SetScene(const SharedPtr<Scene>& scene);

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
        HashMap<Entity, HashSet<ComponentID>> m_CollapsedComponents{Allocator::Persistent};
        HashMap<ComponentID, CustomInspector> m_CustomInspectors{Allocator::Persistent};

        SharedPtr<Scene> m_Scene;
        SceneWorkspace& m_SceneWorkspace;
        UndoSystem& m_UndoSystem;
    };
}
