#include "qspch.h"
#include "SceneSerializer.h"

#include "Quelos/Serialization/SceneBinarySerializer.h"

using namespace magic_enum::bitwise_operators;

namespace Quelos {
    using namespace Serialization;

    static constexpr std::string s_SceneFileExtension = ".qscene";
    static const std::filesystem::path s_ScenePatchesFolder = "Patches";
    static constexpr std::string s_ScenePatchFileExtension = ".qpatch";

    SceneSerializer::SceneSerializer(const Ref<Scene>& scene, const std::filesystem::path& sceneFolderPath)
        : m_Scene(scene), m_ScenePath(sceneFolderPath) {
        if (!std::filesystem::is_directory(sceneFolderPath)) {
            if (std::filesystem::exists(sceneFolderPath)) {
                QS_ERROR_TAG("SceneSerializer", "Scene folder path is not a directory");
                return;
            }

            std::filesystem::create_directories(sceneFolderPath);
        }

        std::filesystem::path patchesFolder = sceneFolderPath / s_ScenePatchesFolder;
        if (!std::filesystem::exists(patchesFolder)) {
            std::filesystem::create_directories(patchesFolder);
        }

        std::filesystem::path sceneFilePath = sceneFolderPath / (sceneFolderPath.filename().string() +
            s_SceneFileExtension);

        if (!std::filesystem::exists(sceneFilePath)) {
            std::ofstream create(sceneFilePath, std::ios::binary);
            SceneHeader sceneHeader;
            create.write(reinterpret_cast<const char*>(&sceneHeader), sizeof(SceneHeader));
        }

        flecs::world& world = scene->GetWorld();
        ComponentRegistry& registry = scene->GetComponentRegistry();

        std::ifstream file(
            sceneFilePath,
            std::ios::binary | std::ios::ate
        );

        if (!file) {
            QS_ERROR_TAG("SceneSerializer", "Failed to open scene file '{}'", sceneFilePath.string());
            return;
        }

        const size_t fileSize = file.tellg();
        Vec<byte> buffer(fileSize);

        file.seekg(0);
        file.read(reinterpret_cast<char*>(buffer.data()), fileSize);

        BinaryReader reader(buffer);

        // Read header
        SceneHeader header = reader.Read<SceneHeader>().value();

        if (header.Magic != SceneHeader().Magic) {
            QS_ERROR_TAG("SceneSerializer", "Invalid scene magic!");
            return;
        }

        for (uint64_t i = 0; i < header.EntityCount; i++) {
            ActorID entityId(reader.Read<uint64_t>().value());
            uint32_t nameLength = reader.Read<uint32_t>().value();
            auto nameBytes = reader.ReadBytes(nameLength).value();

            std::string name(
                reinterpret_cast<const char*>(nameBytes.data()),
                nameLength
            );

            Entity entity = scene->CreateActor(entityId, name);

            uint32_t componentCount = reader.Read<uint32_t>().value();
            for (uint32_t c = 0; c < componentCount; c++) {
                ComponentID guid(reader.Read<uint64_t>().value());

                auto typeInfo = registry.GetSerializableComponentInfo(guid);

                if (!typeInfo.Guid.IsValid()) {
                    QS_ERROR_TAG("SceneSerializer", "Unknown component type in scene!");
                    continue;
                }

                // Serialized blob size
                uint64_t blobSize = reader.Read<uint64_t>().value();

                std::span<const std::byte> blob = reader.ReadBytes(blobSize).value();

                // Allocate component
                void* componentPtr = ecs_ensure_id(world.c_ptr(), entity.GetID(), typeInfo.RuntimeID, 0);

                if (!componentPtr) {
                    QS_ERROR_TAG("SceneSerializer", "Failed to ensure component!");
                    continue;
                }

                // Deserialize
                BinaryReader subReader(blob);
                BinaryReadArchive archive(subReader);
                typeInfo.SerializeBinaryReadFunc(archive, componentPtr);
            }
        }

        for (auto& patchFileEntry : std::filesystem::directory_iterator(patchesFolder)) {
            const auto& patchFilePath = patchFileEntry.path();
            if (!patchFileEntry.is_regular_file() || patchFilePath.extension() != s_ScenePatchFileExtension) {
                QS_INFO(
                    "invalid file '{}': (isFile: {}, extension: {})",
                    patchFilePath.string(),
                    patchFileEntry.is_regular_file(),
                    patchFilePath.extension().string()
                );

                continue;
            }

            std::ifstream patchFile(patchFilePath, std::ios::binary | std::ios::ate);

            if (!patchFile) {
                QS_ERROR_TAG("SceneSerializer", "Failed to open scene patch file!");
                continue;
            }

            const size_t patchFileSize = patchFile.tellg();
            patchFile.seekg(0);

            std::string patchFileBuffer;
            patchFileBuffer.resize(patchFileSize);
            patchFile.read(patchFileBuffer.data(), patchFileSize);

            m_Reader = QuelReader(patchFileBuffer);

            for (auto&& parserEvent : m_Reader.Parse()) {
                std::visit([this]<typename TEvent>(const TEvent& e) {
                    using T = std::decay_t<TEvent>;

                    if (m_SkipToNextSection) {
                        if constexpr (std::is_same_v<T, SectionEvent>) {
                            m_SkipToNextSection = false;
                        }
                        else {
                            return;
                        }
                    }

                    this->OnEvent(e);
                }, parserEvent);
            }

            DeserializeComponentData();
            m_ParserState &= ~ParserState::InComponent;
        }
    }

    void SceneSerializer::DeserializeComponentData() {
        if (!m_CurrentComponentID.IsValid()) {
            return;
        }

        const flecs::world& world = m_Scene->GetWorld();
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

    void SceneSerializer::OnEvent(const SectionEvent& e) {
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

    void SceneSerializer::OnEvent(const ComponentEvent& e) {
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

    void SceneSerializer::OnEvent(const FieldEvent& e) {
        m_CurrentField = e.Path;
    }

    void SceneSerializer::OnEvent(const ValueEvent& e) {
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
                        m_CurrentEntityID = ActorID(std::get<std::string_view>(e.Value));
                    }
                }
                else if (m_CurrentField == "state") {
                    if (std::holds_alternative<std::string_view>(e.Value)) {
                        m_CurrentEntityState = std::get<std::string_view>(e.Value);
                    }
                }

                if (m_CurrentEntityID.IsValid() && !m_CurrentEntityName.empty() && !m_CurrentEntityState.empty()) {
                    if (m_CurrentEntityState == "added") {
                        m_CurrentEntity = m_Scene->CreateActor(m_CurrentEntityID, m_CurrentEntityName);
                    }
                    else if (m_CurrentEntityState == "changed") {
                        m_CurrentEntity = m_Scene->GetEntity(m_CurrentEntityID);
                    }
                    else if (m_CurrentEntityState == "removed") {
                        m_Scene->DestroyEntity(m_CurrentEntityID);
                        m_SkipToNextSection = true;
                    }

                    m_CurrentEntityID = {};
                    m_CurrentEntityName = "";
                    m_CurrentEntityState = "";
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
            }
            else {
                PushBackToContainer(index);
            }
        }
    }

    void SceneSerializer::OnEvent(const TupleBeginEvent& e) {
        m_ParserState |= ParserState::InTuple;
        TextArchiveValue value;
        value.Data = TupleValue{};

        size_t index = m_ValuePool.size();
        m_ValuePool.push_back(value);

        if (m_ContainerStack.empty()) {
            m_FieldTable.emplace_back(m_CurrentField, index);
        }
        else {
            PushBackToContainer(index);
        }

        m_ContainerStack.push_back(index);
    }

    void SceneSerializer::OnEvent(const TupleEndEvent& e) {
        m_ParserState &= ~ParserState::InTuple;

        m_ContainerStack.pop_back();
    }

    void SceneSerializer::OnEvent(const ArrayBeginEvent& e) {
        m_ParserState |= ParserState::InArray;

        TextArchiveValue value;
        value.Data = ArrayValue{};

        size_t index = m_ValuePool.size();
        m_ValuePool.push_back(value);

        if (m_ContainerStack.empty()) {
            m_FieldTable.emplace_back(m_CurrentField, index);
        }
        else {
            PushBackToContainer(index);
        }

        m_ContainerStack.push_back(index);
    }

    void SceneSerializer::OnEvent(const ArrayEndEvent& e) {
        m_ParserState &= ~ParserState::InArray;

        m_ContainerStack.pop_back();
    }

    void SceneSerializer::OnEvent(const ParseError& e) {
        QS_CORE_ERROR_TAG("Serialization", "Parser error in line '{}': {}", e.Line, e.Message);
    }

    void SceneSerializer::PushBackToContainer(const size_t childIndex) {
        const size_t parentIndex = m_ContainerStack.back();
        TextArchiveValue& parent = m_ValuePool[parentIndex];

        if (auto* tuple = std::get_if<TupleValue>(&parent.Data)) {
            tuple->Elements.push_back(childIndex);
        }
        else if (auto* array = std::get_if<ArrayValue>(&parent.Data)) {
            array->Elements.push_back(childIndex);
        }
    }

    static std::string_view GetStateName(const EntityPatch::State state) {
        switch (state) {
        case EntityPatch::State::Changed: return "changed";
        case EntityPatch::State::Added: return "added";
        case EntityPatch::State::Removed: return "removed";
        }

        return "added";
    }

    void SceneSerializer::SerializePatches() {
        flecs::world& world = m_Scene->GetWorld();
        ComponentRegistry& registry = m_Scene->GetComponentRegistry();

        std::filesystem::path patchesFolder = m_ScenePath / s_ScenePatchesFolder;
        for (auto& [entity, patch] : m_Entities) {
            std::string guid = entity.Get<Actor>().ID.ToString();

            std::filesystem::path filePath = patchesFolder / (guid + s_ScenePatchFileExtension);
            std::ifstream patchReadFile(filePath, std::ios::binary | std::ios::ate);

            if (patchReadFile) {
                const size_t patchFileSize = patchReadFile.tellg();
                patchReadFile.seekg(0);

                std::string patchFileBuffer;
                patchFileBuffer.resize(patchFileSize);
                patchReadFile.read(patchFileBuffer.data(), patchFileSize);

                m_Reader = QuelReader(patchFileBuffer);

                RuntimeID componentID = 0;
                EntityPatch::State previousPatchState;
                SectionKind sectionKind;
                ParserState parserState;
                std::string_view sectionField;

                for (auto&& parserEvent : m_Reader.Parse()) {
                    std::visit([&]<typename TEvent>(const TEvent& e) {
                        using T = std::decay_t<TEvent>;

                        if constexpr (std::is_same_v<T, SectionEvent>) {
                            parserState = ParserState::InSection;
                            if (e.Name == "entity") {
                                sectionKind = SectionKind::Entity;
                            }
                            else if (e.Name == "scene") {
                                sectionKind = SectionKind::SceneHeader;
                            }
                            else {
                                sectionKind = SectionKind::None;
                            }

                            componentID = 0;
                        }
                        else if constexpr (std::is_same_v<T, ComponentEvent>) {
                            parserState |= ParserState::InComponent;
                            componentID = registry.GetSerializableComponentInfo(
                                                      ComponentRegistry::GetComponentID(e.Name))
                                                  .RuntimeID;
                        }
                        else if constexpr (std::is_same_v<T, FieldEvent>) {
                            if ((parserState & ParserState::InComponent) == ParserState::None) {
                                sectionField = e.Path;
                                return;
                            }

                            if ((parserState & (ParserState::InArray | ParserState::InTuple)) != ParserState::None) {
                                return;
                            }

                            patch.Components[componentID].Fields.emplace(e.Path);
                        }
                        else if constexpr (std::is_same_v<T, TupleBeginEvent>) {
                            parserState |= ParserState::InTuple;
                        }
                        else if constexpr (std::is_same_v<T, TupleEndEvent>) {
                            parserState &= ~ParserState::InTuple;
                        }
                        else if constexpr (std::is_same_v<T, ArrayBeginEvent>) {
                            parserState |= ParserState::InArray;
                        }
                        else if constexpr (std::is_same_v<T, ArrayEndEvent>) {
                            parserState &= ~ParserState::InArray;
                        }
                        else if constexpr (std::is_same_v<T, ValueEvent>) {
                            if ((parserState & ParserState::InSection) == ParserState::None
                                || sectionKind != SectionKind::Entity
                            ) {
                                return;
                            }

                            if ((parserState & ParserState::InComponent) != ParserState::None) {
                                return;
                            }

                            if (sectionField == "state") {
                                if (std::holds_alternative<std::string_view>(e.Value)) {
                                    const std::string_view state = std::get<std::string_view>(e.Value);

                                    if (state == "added") {
                                        previousPatchState = EntityPatch::State::Added;
                                    }
                                    else if (state == "removed") {
                                        previousPatchState = EntityPatch::State::Removed;
                                    }
                                    else if (state == "changed") {
                                        previousPatchState = EntityPatch::State::Changed;
                                    }
                                }
                            }
                        }
                    }, parserEvent);
                }

                patch.ApplyPreviousState(previousPatchState);
            }

            std::string stringBuffer;
            StringQuelWriter quelWriter(stringBuffer);
            quelWriter.SetIndent(2);

            quelWriter.Write(SectionEvent{"entity"});
            quelWriter.WriteField("guid", guid);
            quelWriter.WriteField("name", std::string_view(entity.GetName()));
            quelWriter.WriteField("state", GetStateName(patch.PatchState));

            quelWriter.CloseSection();

            if (flecs::entity parent = entity.GetID().target(flecs::ChildOf)) {
                if (const auto* runtimeTag = parent.try_get<Actor>()) {
                    quelWriter.WriteField("parent", runtimeTag->ID.ToString());
                }
            }

            for (auto& [componentId, compPatch] : patch.Components) {
                SerializableComponentInfo* info = registry.GetSerializableComponentInfo(componentId);
                if (!info) {
                    continue;
                }

                void* ptr = ecs_get_mut_id(world.c_ptr(), entity.GetID(), info->RuntimeID);
                if (!ptr) {
                    continue;
                }

                quelWriter.Write(ComponentEvent{info->Name});

                QuelWriteArchive archive(quelWriter, &compPatch.Fields);
                info->SerializeTextWriteFunc(archive, ptr);
            }

            std::ofstream file(filePath, std::ios::binary);
            if (!file) {
                QS_CORE_ERROR("Failed to open file: {}", filePath.string());
                continue;
            }

            file.write(
                stringBuffer.data(),
                static_cast<std::streamsize>(stringBuffer.size())
            );
        }

        m_Entities.clear();
    }

    void SceneSerializer::BakePatches() {
    }

    void SceneSerializer::PushComponentFieldCommand(Entity entity, RuntimeID componentId, std::string_view field) {
    }
}
