#pragma once

#include "Quelos/Scenes/Scene.h"
#include "SetFieldCommand.h"

#include "Quelos/Core/Base.h"

namespace Quelos {
    struct ComponentPatch {
        ComponentUntypedRef ComponentRef;
        HashSet<std::string_view> Fields;
    };

    struct EntityPatch {
        enum class State : uint8_t {
            Changed = 0,
            Added = 1,
            Removed = 2
        };

        OrderedMap<RuntimeID, ComponentPatch> Components;
        State PatchState = State::Changed;

        void SetState(const State next) {
            if (PatchState == State::Added && next == State::Changed) {
                return;
            }

            PatchState = next;
        }

        void ApplyPreviousState(const State previous) {
            // Hacking but idk
            const State current = PatchState;
            PatchState = previous;
            SetState(current);
        }
    };

    class SceneSerializer {
    public:
        SceneSerializer() = default;
        SceneSerializer(const Ref<Scene>& scene, const std::filesystem::path& sceneFolderPath);
        ~SceneSerializer() = default;

        void SerializePatches();
        void BakePatches();

        void PushComponentFieldCommand(Entity entity, RuntimeID componentId, std::string_view field);

        template<typename T>
        void Record(SetField<T>& cmd) {
            const Entity entity(cmd.ComponentRef.GetEntityID());

            auto& entityPatch = m_Entities[entity.GetID()];
            auto& compPatch = entityPatch.Components[cmd.ComponentRef.GetID()];

            compPatch.ComponentRef = cmd.ComponentRef;
            compPatch.Fields.insert(cmd.FieldKey);

            entityPatch.SetState(EntityPatch::State::Changed);
        }

    private:
        Ref<Scene> m_Scene;
        std::filesystem::path m_ScenePath;

    private:
        void DeserializeComponentData();

        void OnEvent(const Serialization::SectionEvent& e);
        void OnEvent(const Serialization::ComponentEvent& e);
        void OnEvent(const Serialization::FieldEvent& e);
        void OnEvent(const Serialization::ValueEvent& e);
        void OnEvent(const Serialization::TupleBeginEvent& e);
        void OnEvent(const Serialization::TupleEndEvent& e);
        void OnEvent(const Serialization::ArrayBeginEvent& e);
        void OnEvent(const Serialization::ArrayEndEvent& e);

        static void OnEvent(const Serialization::ParseError& e);

        void PushBackToContainer(size_t childIndex);

    private:
        HashMap<Entity, EntityPatch> m_Entities{};

        enum class ParserState : uint8_t {
            None = 0,
            InSection = 1,
            InComponent = 2,
            InArray = 4,
            InTuple = 8
        };

        enum class SectionKind : uint8_t {
            None,
            SceneHeader,
            Entity
        };

        Serialization::QuelReader m_Reader;

        ParserState m_ParserState = ParserState::None;
        SectionKind m_SectionKind = SectionKind::None;

        std::string_view m_CurrentSection;

        Entity m_CurrentEntity;
        std::string_view m_CurrentEntityName;
        std::string_view m_CurrentEntityState;
        ActorID m_CurrentEntityID{};

        bool m_SkipToNextSection = false;

        std::string_view m_CurrentComponent;
        ComponentID m_CurrentComponentID{};

        std::string_view m_CurrentField;

        std::unordered_map<std::string_view, Serialization::TextArchiveValue> m_FieldsMap;

        std::vector<Serialization::TextArchiveValue> m_ValuePool;
        std::vector<std::pair<std::string_view, size_t>> m_FieldTable;

        std::vector<size_t> m_ContainerStack;

        bool hadError = false;
    };
}
