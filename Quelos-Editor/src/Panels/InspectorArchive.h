#pragma once

#include "Quelos/Scenes/Scene.h"

#include "UndoSystem.h"

namespace Quelos {
    class InspectorArchive {
    public:
        static constexpr bool IsLoading = false;
        static constexpr bool IsSaving = true;

    public:
        InspectorArchive(
            const Entity& entity,
            flecs::id componentID,
            Ref<Scene>& scene,
            UndoSystem& undoSystem,
            std::move_only_function<void(SetFieldArchive& archive, void* data)> serializeComponentFunc
        ) : m_Entity(entity), m_ComponentID(componentID), m_Scene(scene), m_UndoSystem(undoSystem)
        {
            m_SerializeComponentFunc = std::move(serializeComponentFunc);
        }

        template <typename T>
        void Field(std::string_view name, T& value) {
            DrawField(name, value);
        }

    private:
        void DrawField(std::string_view name, glm::vec3& value);
        void DrawField(std::string_view name, glm::quat& value);

    private:
        Entity m_Entity;
        flecs::id m_ComponentID;
        Ref<Scene>& m_Scene;
        std::move_only_function<void(SetFieldArchive& archive, void* data)> m_SerializeComponentFunc = nullptr;
        UndoSystem& m_UndoSystem;
    };
}
