#pragma once

#include "Quelos/Scenes/Scene.h"

#include "../Scene/Commands/SetFieldCommand.h"
#include "UndoSystem.h"

#include "Quelos/ImGui/ImGuiUI.h"

namespace Quelos {
    inline std::string BeautifyLabel(std::string label) {
        if (label.rfind("m_", 0) == 0) {
            label.erase(0, 2);
        }
        else if (!label.empty() && label[0] == '_') {
            label.erase(0, 1);
        }

        if (!label.empty()) {
            label[0] = static_cast<char>(std::toupper(label[0]));
        }

        return label;
    }

    class BeatifyFieldNamesArchive {
    public:
        static constexpr bool IsLoading = false;
        static constexpr bool IsSaving = true;
    public:
        template <typename T>
        void Field(const std::string_view name, T&) {
            FieldNames[Serialization::GetPathID(name)] = BeautifyLabel(std::string(name));
        }

        template <typename T>
        void FieldVector(const std::string_view name, std::vector<T>&) {
            FieldNames[Serialization::GetPathID(name)] = BeautifyLabel(std::string(name));
        }
    public:
        HashMap<Serialization::PathID, std::string>& FieldNames;
    };

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
            SetFieldSerializeFn serializeComponentFunc,
            const HashMap<Serialization::PathID, std::string>& formattedFieldNames
        ) : m_Actor(entity), m_ComponentID(componentID), m_Scene(scene), m_UndoSystem(undoSystem), m_FormattedFieldNames(formattedFieldNames)
        {
            m_SerializeComponentFunc = serializeComponentFunc;
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

                if (UI::EditEnum(GetFormattedFieldName(name), temp)) {
                    m_UndoSystem.Push<SetField<T>>(
                        m_Scene->GetComponentRegistry().GetSerializableComponentInfo(m_ComponentID)->Guid,
                        m_Actor.GetActorID(),
                        m_Scene,
                        m_SerializeComponentFunc,
                        name,
                        value,
                        temp
                    );

                    value = temp;
                }
            }
        }
        template<typename T>
        void DrawField(std::string_view name, Ref<T>& value) {
            if constexpr (std::is_base_of_v<Asset, T>) {
                Ref<T> temp = value;

                if (UI::EditAsset(GetFormattedFieldName(name), temp)) {
                    m_UndoSystem.Push<SetField<Ref<T>>>(
                        m_Scene->GetComponentRegistry().GetSerializableComponentInfo(m_ComponentID)->Guid,
                        m_Actor.GetActorID(),
                        m_Scene,
                        m_SerializeComponentFunc,
                        name,
                        value,
                        temp
                    );
                }
            }
        }

        void DrawField(std::string_view name, float& value);
        void DrawField(std::string_view name, glm::vec3& value);
        void DrawField(std::string_view name, glm::quat& value);

        const std::string& GetFormattedFieldName(const std::string_view name) const {
            return m_FormattedFieldNames.at(Serialization::GetPathID(name));
        }

    private:
        Actor m_Actor;
        flecs::id m_ComponentID;
        Ref<Scene>& m_Scene;
        SetFieldSerializeFn m_SerializeComponentFunc = nullptr;
        UndoSystem& m_UndoSystem;
        const HashMap<Serialization::PathID, std::string>& m_FormattedFieldNames;
    };
}
