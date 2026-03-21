#pragma once

#include "Commands/CreateActorCommand.h"
#include "Quelos/Scenes/Scene.h"
#include "Commands/SetFieldCommand.h"
#include "Commands/SetParentCommand.h"

#include "Quelos/Core/Base.h"

namespace Quelos {
    struct ComponentPatch {
        HashMap<std::string_view, uint32_t> Fields;
    };

    struct ActorPatch {
        enum class State : uint8_t {
            Changed = 0,
            Added = 1,
            Removed = 2
        };

        OrderedMap<ComponentID, ComponentPatch> Components;
        uint32_t ParentPatchCount = 0;
        Deque<State> PatchStates;

        void StatePushBack(const State next) {
            PatchStates.push_back(next);
        }

        void PushFrontState(const State previous) {
            PatchStates.push_front(previous);
        }

        void StatePopBack() {
            PatchStates.pop_back();
        }

        void PopFrontState() {
            PatchStates.pop_front();
        }
    };

    struct ParentPair {
        ActorID ChildID;
        ActorID ParentID;
    };

    class SceneSerializer {
    public:
        SceneSerializer() = default;
        SceneSerializer(const Ref<Scene>& scene, const std::filesystem::path& sceneFolderPath);
        ~SceneSerializer() = default;

        void SerializePatches();
        void BakePatches() const;

        template<typename T>
        void Record(SetField<T>& cmd) {
            auto& entityPatch = m_Actors[cmd.ActorId];
            auto& compPatch = entityPatch.Components[cmd.ComponentId];

            ++compPatch.Fields[cmd.FieldKey];

            entityPatch.StatePushBack(ActorPatch::State::Changed);
        }

        template<typename T>
        void Remove(SetField<T>& cmd) {
            auto& entityPatch = m_Actors[cmd.ActorId];
            auto& compPatch = entityPatch.Components[cmd.ComponentId];

            --compPatch.Fields[cmd.FieldKey];

            entityPatch.StatePopBack();
        }

        void Record(const SetParent& cmd) {
            auto& entityPatch = m_Actors[cmd.ActorId];
            entityPatch.ParentPatchCount++;
            entityPatch.StatePushBack(ActorPatch::State::Changed);
        }

        void Remove(const SetParent& cmd) {
            auto& entityPatch = m_Actors[cmd.ActorId];
            entityPatch.ParentPatchCount--;
            entityPatch.StatePopBack();
        }

        void Record(const CreateActor& cmd) {
            auto& entityPatch = m_Actors[cmd.ActorId];
            entityPatch.StatePushBack(ActorPatch::State::Added);
        }

        void Remove(const CreateActor& cmd) {
            auto& entityPatch = m_Actors[cmd.ActorId];
            entityPatch.StatePopBack();
        }

        void Record(const DestroyActor& cmd) {
            auto& entityPatch = m_Actors[cmd.ActorId];
            entityPatch.StatePushBack(ActorPatch::State::Removed);
        }

        void Remove(const DestroyActor& cmd) {
            auto& entityPatch = m_Actors[cmd.ActorId];
            entityPatch.StatePopBack();
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
        HashMap<ActorID, ActorPatch> m_Actors{};

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

        HashMap<std::string_view, Serialization::TextArchiveValue> m_FieldsMap;

        Vec<ParentPair> m_ParentPairsToResolve;

        Vec<Serialization::TextArchiveValue> m_ValuePool;
        Vec<Pair<std::string_view, size_t>> m_FieldTable;

        Vec<size_t> m_ContainerStack;

        bool hadError = false;
    };
}
