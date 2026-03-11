#pragma once

#include "Quelos/Scenes/Scene.h"

#include "UndoSystem.h"

#include "Quelos/ImGui/ImGuiUI.h"

namespace Quelos {
    class InspectorArchive {
    public:
        static constexpr bool IsLoading = true;
        static constexpr bool IsSaving = true;

    public:
        InspectorArchive(
            const Entity& entity,
            flecs::id componentID,
            Ref<Scene>& scene,
            UndoSystem& undoSystem,
            SetFieldSerializeFn serializeComponentFunc
        ) : m_Entity(entity), m_ComponentID(componentID), m_Scene(scene), m_UndoSystem(undoSystem)
        {
            m_SerializeComponentFunc = std::move(serializeComponentFunc);
        }

        template <typename T>
        void Field(std::string_view name, T& value) {
            DrawField(name, value);
        }

        // TODO:
        template <typename T>
            requires(std::is_trivially_copyable_v<T>)
        void FieldVector(const std::string&, std::vector<T>& data) { }
        template <typename T>
        void Value(T& value) { }
        static void BeginTuple() { }
        static void BeginTupleField(const std::string_view) { }
        static void EndTuple() { }

    private:
        template<typename T>
        void DrawField(std::string_view name, T& value) {
            if constexpr (std::is_enum_v<T>) {
                T temp = value;

                if (UI::EditEnum(std::string(name), temp)) {
                    m_UndoSystem.Push<SetField<T>>(
                        m_Entity.GetUntypedRef(m_ComponentID),
                        m_SerializeComponentFunc,
                        name,
                        value,
                        temp
                    );

                    value = temp;
                }
            }
        }

        void DrawField(std::string_view name, float& value);
        void DrawField(std::string_view name, glm::vec3& value);
        void DrawField(std::string_view name, glm::quat& value);

    private:
        Entity m_Entity;
        flecs::id m_ComponentID;
        Ref<Scene>& m_Scene;
        SetFieldSerializeFn m_SerializeComponentFunc = nullptr;
        UndoSystem& m_UndoSystem;
    };
}
