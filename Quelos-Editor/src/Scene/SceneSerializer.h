#pragma once

#include "Commands/ComponentCommand.h"
#include "Commands/CreateActorCommand.h"
#include "Commands/ReorderChildrenCommand.h"
#include "Commands/SetEntityNameCommnad.h"
#include "Quelos/Scenes/Scene.h"
#include "Commands/SetFieldCommand.h"
#include "Commands/SetParentCommand.h"

#include "Quelos/Core/Base.h"

namespace Quelos {
    enum class PatchState : uint8_t {
        Changed = 0,
        Added = 1,
        Removed = 2
    };

    struct ComponentPatch {
        HashMap<std::string_view, uint32_t> Fields;
        Deque<PatchState> PatchStates;

        void StatePushBack(const PatchState next) {
            PatchStates.push_back(next);
        }

        void PushFrontState(const PatchState previous) {
            PatchStates.push_front(previous);
        }

        void StatePopBack() {
            PatchStates.pop_back();
        }

        void PopFrontState() {
            PatchStates.pop_front();
        }
    };

    struct ActorPatch {
        OrderedMap<ComponentID, ComponentPatch> Components;
        uint32_t ParentPatchCount = 0;
        Deque<PatchState> PatchStates;

        void StatePushBack(const PatchState next) {
            PatchStates.push_back(next);
        }

        void PushFrontState(const PatchState previous) {
            PatchStates.push_front(previous);
        }

        void StatePopBack() {
            PatchStates.pop_back();
        }

        void PopFrontState() {
            PatchStates.pop_front();
        }
    };

    struct ChildEntry {
        Actor Child;
        uint64_t Order = 0;
    };

    class SceneSerializer {
    public:
        static inline const std::string SceneFileExtension = ".qscene";
        static inline const std::filesystem::path ScenePatchesFolder = "Patches";
        static inline const std::string ScenePatchFileExtension = ".qpatch";
    public:
        SceneSerializer() = default;
        SceneSerializer(const Ref<Scene>& scene, const Path& sceneFolderPath);
        ~SceneSerializer() = default;

        void Deserialize();

        void SerializePatches();
        void BakePatches() const;

        template<typename T>
        void Record(SetField<T>& cmd) {
            auto& entityPatch = m_Actors[cmd.ActorId];
            auto& compPatch = entityPatch.Components[cmd.ComponentId];

            compPatch.StatePushBack(PatchState::Changed);
            ++compPatch.Fields[cmd.FieldKey];

            entityPatch.StatePushBack(PatchState::Changed);
        }

        template<typename T>
        void Remove(SetField<T>& cmd) {
            auto& entityPatch = m_Actors[cmd.ActorId];
            auto& compPatch = entityPatch.Components[cmd.ComponentId];

            compPatch.StatePopBack();
            --compPatch.Fields[cmd.FieldKey];

            entityPatch.StatePopBack();
        }

        void Record(const AddComponentCommand& cmd) {
            auto& entityPatch = m_Actors[cmd.ActorId];
            entityPatch.Components[cmd.ComponentId].StatePushBack(PatchState::Added);
            entityPatch.StatePushBack(PatchState::Changed);
        }

        void Remove(const AddComponentCommand& cmd) {
            auto& entityPatch = m_Actors[cmd.ActorId];
            entityPatch.Components[cmd.ComponentId].StatePopBack();
            entityPatch.StatePopBack();
        }

        void Record(const RemoveComponentCommand& cmd) {
            auto& entityPatch = m_Actors[cmd.ActorId];
            entityPatch.Components[cmd.ComponentId].StatePushBack(PatchState::Removed);
            entityPatch.StatePushBack(PatchState::Changed);
        }

        void Remove(const RemoveComponentCommand& cmd) {
            auto& entityPatch = m_Actors[cmd.ActorId];
            entityPatch.Components[cmd.ComponentId].StatePopBack();
            entityPatch.StatePopBack();
        }

        void Record(const SetEntityName& cmd) {
            auto& entityPatch = m_Actors[cmd.ActorId];
            entityPatch.StatePushBack(PatchState::Changed);
        }

        void Remove(const SetEntityName& cmd) {
            auto& entityPatch = m_Actors[cmd.ActorId];
            entityPatch.StatePopBack();
        }

        void Record(const SetParent& cmd) {
            auto& entityPatch = m_Actors[cmd.ActorId];
            entityPatch.ParentPatchCount++;
            entityPatch.StatePushBack(PatchState::Changed);
        }

        void Remove(const SetParent& cmd) {
            auto& entityPatch = m_Actors[cmd.ActorId];
            entityPatch.ParentPatchCount--;
            entityPatch.StatePopBack();
        }

        void Record(const ReorderChild& cmd) {
            ActorPatch& actorPatch = m_Actors[cmd.ActorId];
            actorPatch.ParentPatchCount++;
            actorPatch.StatePushBack(PatchState::Changed);
        }

        void Remove(const ReorderChild& cmd) {
            auto& entityPatch = m_Actors[cmd.ActorId];
            entityPatch.ParentPatchCount--;
            entityPatch.StatePopBack();
        }

        void Record(const CreateActor& cmd) {
            auto& entityPatch = m_Actors[cmd.ActorId];
            entityPatch.StatePushBack(PatchState::Added);
        }

        void Remove(const CreateActor& cmd) {
            auto& entityPatch = m_Actors[cmd.ActorId];
            entityPatch.StatePopBack();
        }

        void Record(const DestroyActor& cmd) {
            auto& entityPatch = m_Actors[cmd.ActorId];
            entityPatch.StatePushBack(PatchState::Removed);
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
            Actor
        };

        Serialization::QuelReader m_Reader;

        ParserState m_ParserState = ParserState::None;
        SectionKind m_SectionKind = SectionKind::None;

        std::string_view m_CurrentSection;

        Actor m_CurrentEntity;
        std::string_view m_CurrentEntityName;
        std::string_view m_CurrentEntityState;
        ActorID m_CurrentEntityID{};
        ActorID m_CurrentParentID{};

        bool m_SkipToNextSection = false;
        bool m_SkipToNextComponent = false;

        std::string_view m_CurrentComponent;
        ComponentID m_CurrentComponentID{};

        std::string_view m_CurrentField;
        bool m_IsFirstComponentField = true;

        HashMap<std::string_view, Serialization::TextArchiveValue> m_FieldsMap;

        HashMap<ActorID, Vec<ChildEntry>> m_ParentPairsToResolve;

        Vec<Serialization::TextArchiveValue> m_ValuePool;
        Vec<Pair<std::string_view, size_t>> m_FieldTable;

        Vec<size_t> m_ContainerStack;

        bool hadError = false;
    };
}
