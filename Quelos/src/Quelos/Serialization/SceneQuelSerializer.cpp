#include "qspch.h"
#include "SceneQuelSerializer.h"

#include "flecs.h"

#include "Quelos/Scenes/Scene.h"
#include "Quelos/Scenes/ComponentRegistery.h"

#include "magic_enum/magic_enum.hpp"
using namespace magic_enum::bitwise_operators;

template <class... Ts>
struct Overloaded : Ts... {
    using Ts::operator()...;
};

template <class... Ts>
Overloaded(Ts...) -> Overloaded<Ts...>;

namespace Quelos::Serialization {
    void SceneQuelSerializer::Deserialize(const Ref<Scene>& scene, const std::filesystem::path& path) {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file) {
            throw std::runtime_error("Failed to open scene file");
        }

        const size_t fileSize = file.tellg();
        file.seekg(0);

        std::string buffer;
        buffer.resize(fileSize);
        file.read(buffer.data(), fileSize);

        m_Scene = scene;
        m_Reader = QuelReader(buffer);

        for (auto&& parserEvent : m_Reader.Parse()) {
            std::visit([this](const auto& e) { this->OnEvent(e); }, parserEvent);
        }

        DeserializeComponentData();
        m_ParserState &= ~ParserState::InComponent;
    }

    void SceneQuelSerializer::DeserializeComponentData() {
        if (!m_CurrentComponentID.IsValid()) {
            return;
        }

        flecs::world& world = m_Scene->GetWorld();
        auto& registry = m_Scene->GetComponentRegistry();
        const SerializableComponentInfo componentInfo = registry.GetSerializableComponentInfo(m_CurrentComponentID);

        if (!componentInfo.Guid.IsValid()) {
            return;
        }

        QuelReadArchive archive(m_FieldTable, m_ValuePool);
        ecs_add_id(world.c_ptr(), m_CurrentEntity.GetID(), componentInfo.RuntimeID);
        void* data = ecs_get_mut_id(world.c_ptr(), m_CurrentEntity.GetID(), componentInfo.RuntimeID);

        if (!data) {
            return;
        }

        componentInfo.SerializeTextReadFunc(archive, data);

        m_FieldsMap.clear();
        m_ContainerStack.clear();
    }

    void SceneQuelSerializer::OnEvent(const SectionEvent& e) {
        DeserializeComponentData();

        m_ParserState = ParserState::InSection;

        m_CurrentEntityName = "";
        m_CurrentEntityID = {};
        m_CurrentComponent = "";

        if (e.Name == "scene") {
            m_SectionKind = SectionKind::SceneHeader;
        }
        else if (e.Name == "entity") {
            m_SectionKind = SectionKind::Entity;
        }
        else {
            m_SectionKind = SectionKind::None;
        }
    }

    void SceneQuelSerializer::OnEvent(const ComponentEvent& e) {
        DeserializeComponentData();

        m_ParserState |= ParserState::InComponent;

        m_ValuePool.clear();
        m_FieldTable.clear();
        m_ContainerStack.clear();

        m_ValuePool.reserve(256);
        m_FieldTable.reserve(128);

        m_CurrentComponent = e.Name;
        m_CurrentComponentID = ComponentRegistry::GetComponentID(m_CurrentComponent);
    }

    void SceneQuelSerializer::OnEvent(const FieldEvent& e) {
        m_CurrentField = e.Path;
    }

    void SceneQuelSerializer::OnEvent(const ValueEvent& e) {
        if ((m_ParserState & ParserState::InComponent) == ParserState::None) {
            switch (m_SectionKind) {
            case SectionKind::SceneHeader:
                if (m_CurrentField == "name") {
                    if (const std::string_view* name = std::get_if<std::string_view>(&e.Value)) {
                        m_Scene->SetName(*name);
                    }
                }
                break;
            case SectionKind::Entity:
                if (m_CurrentField == "name") {
                    if (std::holds_alternative<std::string_view>(e.Value)) {
                        m_CurrentEntityName = std::get<std::string_view>(e.Value);
                    }
                }
                else if (m_CurrentField == "guid") {
                    if (std::holds_alternative<std::string_view>(e.Value)) {
                        m_CurrentEntityID = EntityID(std::get<std::string_view>(e.Value));
                    }
                }

                if (m_CurrentEntityID.IsValid() && !m_CurrentEntityName.empty()) {
                    m_CurrentEntity = m_Scene->CreateEntity(m_CurrentEntityID, m_CurrentEntityName);
                    m_CurrentEntityID = {};
                    m_CurrentEntityName = "";
                }
                break;
            default:
                break;
            }
        }
        else {
            TextArchiveValue value;
            value.Data = e.Value;

            size_t index = m_ValuePool.size();
            m_ValuePool.push_back(value);

            if (m_ContainerStack.empty()) {
                m_FieldTable.emplace_back(m_CurrentField, index);
            } else {
                PushBackToContainer(index);
            }
        }
    }

    void SceneQuelSerializer::OnEvent(const TupleBeginEvent& e) {
        m_ParserState |= ParserState::InTuple;
        TextArchiveValue value;
        value.Data = TupleValue{};

        size_t index = m_ValuePool.size();
        m_ValuePool.push_back(value);

        if (m_ContainerStack.empty()) {
            m_FieldTable.emplace_back(m_CurrentField, index);
        } else {
            PushBackToContainer(index);
        }

        m_ContainerStack.push_back(index);
    }

    void SceneQuelSerializer::OnEvent(const TupleEndEvent& e) {
        m_ParserState &= ~ParserState::InTuple;

        m_ContainerStack.pop_back();
    }

    void SceneQuelSerializer::OnEvent(const ArrayBeginEvent& e) {
        m_ParserState |= ParserState::InArray;

        TextArchiveValue value;
        value.Data = ArrayValue{};

        size_t index = m_ValuePool.size();
        m_ValuePool.push_back(value);

        if (m_ContainerStack.empty()) {
            m_FieldTable.emplace_back(m_CurrentField, index);
        } else {
            PushBackToContainer(index);
        }

        m_ContainerStack.push_back(index);
    }

    void SceneQuelSerializer::OnEvent(const ArrayEndEvent& e) {
        m_ParserState &= ~ParserState::InArray;

        m_ContainerStack.pop_back();
    }

    void SceneQuelSerializer::OnEvent(const ParseError& e) {
        QS_CORE_ERROR_TAG("Serialization", "Parser error in line '{}': {}", e.Line, e.Message);
    }

    void SceneQuelSerializer::PushBackToContainer(const size_t childIndex) {
        const size_t parentIndex = m_ContainerStack.back();
        TextArchiveValue& parent = m_ValuePool[parentIndex];

        if (auto* tuple = std::get_if<TupleValue>(&parent.Data)) {
            tuple->Elements.push_back(childIndex);
        }
        else if (auto* array = std::get_if<ArrayValue>(&parent.Data)) {
            array->Elements.push_back(childIndex);
        }
    }

    void SceneQuelSerializer::Serialize(const Ref<Scene>& scene, const std::filesystem::path& path) {
        flecs::world& world = scene->GetWorld();
        ComponentRegistry& registry = scene->GetComponentRegistry();
        const auto& types = registry.GetSerializableComponents();

        std::string stringBuffer;
        StringQuelWriter quelWriter(stringBuffer);
        quelWriter.SetIndent(2);

        // Header
        quelWriter.Write(SectionEvent{ "scene" });
        quelWriter.WriteField("version", static_cast<uint64_t>(1));
        quelWriter.WriteField("name", scene->GetName());

        Map<ecs_id_t, SerializableComponentInfo> idLookup;
        for (auto& typeBuffer : types | std::views::values) {
            idLookup[typeBuffer.RuntimeID] = typeBuffer;
        }

        auto q = world.query_builder<RuntimeTag>().build();

        q.each(
            [&](const flecs::entity entity, const RuntimeTag&) {
                const ecs_entity_t entityId = entity.id();

                quelWriter.Write(SectionEvent{ "entity" });
                quelWriter.WriteField("guid", entity.get<RuntimeTag>().ID.ToString());
                quelWriter.WriteField("name", std::string_view(entity.name()));

                entity.each(
                    [&](const flecs::id id) {
                        const auto it = idLookup.find(id);
                        if (it == idLookup.end()) {
                            return;
                        }

                        const SerializableComponentInfo& componentInfo = it->second;

                        void* ptr = ecs_get_mut_id(world.c_ptr(), entityId, id);
                        if (!ptr) {
                            return;
                        }

                        quelWriter.Write(ComponentEvent{ componentInfo.Name });

                        QuelWriteArchive archive(quelWriter);
                        componentInfo.SerializeTextWriteFunc(archive, ptr);
                    }
                );
            }
        );

        // Disk write
        std::ofstream file(path, std::ios::binary);
        file.write(
            stringBuffer.data(),
            static_cast<std::streamsize>(stringBuffer.size())
        );
    }
}
