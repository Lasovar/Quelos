#pragma once
#include "Serializer.h"
#include "Quelos/Scenes/ComponentRegistery.h"
#include "Quelos/Scenes/Entity.h"

namespace Quelos {
    class Scene;

    namespace Serialization {
        class SceneQuelSerializer {
        public:
            static void Serialize(const Ref<Scene>& scene, const std::filesystem::path& path);
            void Deserialize(const Ref<Scene>& scene, const std::filesystem::path& path);

        private:
            void DeserializeComponentData();

            void OnEvent(const SectionEvent& e);
            void OnEvent(const ComponentEvent& e);
            void OnEvent(const FieldEvent& e);
            void OnEvent(const ValueEvent& e);
            void OnEvent(const TupleBeginEvent& e);
            void OnEvent(const TupleEndEvent& e);
            void OnEvent(const ArrayBeginEvent& e);
            void OnEvent(const ArrayEndEvent& e);

            static void OnEvent(const ParseError& e);

            void PushBackToContainer(size_t childIndex);
        private:
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

            QuelReader m_Reader;

            ParserState m_ParserState = ParserState::None;
            SectionKind m_SectionKind = SectionKind::None;

            std::string_view m_CurrentSection;
            Ref<Scene> m_Scene;

            Entity m_CurrentEntity;
            std::string_view m_CurrentEntityName = "";
            EntityID m_CurrentEntityID{};

            std::string_view m_CurrentComponent;
            ComponentID m_CurrentComponentID{};

            std::string_view m_CurrentField;

            std::unordered_map<std::string_view, TextArchiveValue> m_FieldsMap;

            std::vector<TextArchiveValue> m_ValuePool;
            std::vector<std::pair<std::string_view, size_t>> m_FieldTable;

            std::vector<size_t> m_ContainerStack;

            bool hadError = false;
        };
    }
}
