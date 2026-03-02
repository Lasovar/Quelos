#pragma once

#include "imgui.h"
#include "Quelos/Scenes/Scene.h"

#include "UndoSystem.h"
#include "glm/gtc/type_ptr.hpp"

namespace Quelos {
    class InspectorArchive {
    public:
        static constexpr bool IsLoading = false;
        static constexpr bool IsSaving = true;

    public:
        InspectorArchive(const Entity& entity, Ref<Scene>& scene, UndoSystem& undoSystem)
            : m_Entity(entity), m_Scene(scene), m_UndoSystem(undoSystem) { }

        template<typename T>
        void Field(std::string_view name, T& value) {
            DrawField(name, value);
        }

    private:
        void DrawField(std::string_view name, glm::vec3& value);

        static void DrawField(std::string_view name, glm::quat& value);

    private:
        Entity m_Entity;
        Ref<Scene>& m_Scene;
        UndoSystem& m_UndoSystem;
    };
}
