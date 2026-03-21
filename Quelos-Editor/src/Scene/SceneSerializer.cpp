#include "qspch.h"
#include "SceneSerializer.h"

#include <ranges>

#include "Quelos/Serialization/SceneBinarySerializer.h"

using namespace magic_enum::bitwise_operators;

namespace Quelos {
    using namespace Serialization;

    static const std::string s_SceneFileExtension = ".qscene";
    static const std::filesystem::path s_ScenePatchesFolder = "Patches";
    static const std::string s_ScenePatchFileExtension = ".qpatch";

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

        const flecs::world& world = m_Scene->GetWorld();
        world.defer_begin();

        for (uint64_t i = 0; i < header.EntityCount; i++) {
            const auto entitySnapshotSize = reader.Read<uint32_t>();
            if (!entitySnapshotSize) {
                QS_ERROR_TAG("SceneSerializer", "Invalid entity snapshot size!");
                continue;
            }

            ActorSnapshot actorSnapshot;
            actorSnapshot.Data.resize(entitySnapshotSize.value());
            const auto entitySnapshotResult = reader.ReadBytes(entitySnapshotSize.value());
            if (!entitySnapshotResult) {
                QS_ERROR_TAG("SceneSerializer", "Failed to read entity snapshot!");
                continue;
            }

            Span<const byte> entitySnapshot = entitySnapshotResult.value();
            std::memcpy(actorSnapshot.Data.data(), entitySnapshot.data(), entitySnapshot.size());

            ActorSnapshot::Load(m_Scene, actorSnapshot);
        }

        world.defer_end();

        world.defer_begin();

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

            if (m_CurrentEntity.GetID()) {
                DeserializeComponentData();
            }

            m_ParserState &= ~ParserState::InComponent;

            for (auto& [childId, parentId] : m_ParentPairsToResolve) {
                Entity actor = scene->GetActor(childId);
                if (parentId) {
                    if (const Entity parent = scene->GetActor(parentId); parent.IsValid()) {
                        actor.SetParent(parent);
                    }
                } else {
                    actor.RemoveParent();
                }
            }

            m_ParentPairsToResolve.clear();
        }

        world.defer_end();
    }

    void SceneSerializer::DeserializeComponentData() {
        if (!m_CurrentComponentID.IsValid()) {
            return;
        }

        const flecs::world& world = m_Scene->GetWorld();
        auto& registry = m_Scene->GetComponentRegistry();
        const SerializableComponentInfo* componentInfo = registry.GetSerializableComponentInfo(m_CurrentComponentID);

        if (!componentInfo) {
            return;
        }

        QuelReadArchive archive(m_FieldTable, m_ValuePool);
        ecs_add_id(world.c_ptr(), m_CurrentEntity.GetID(), componentInfo->RuntimeID);
        void* data = ecs_get_mut_id(world.c_ptr(), m_CurrentEntity.GetID(), componentInfo->RuntimeID);

        if (!data) {
            return;
        }

        componentInfo->SerializeTextReadFunc(archive, data);

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
        else if (e.Name == "actor") {
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
                else if (m_CurrentEntityID && m_CurrentField == "parent") {
                    if (std::holds_alternative<std::string_view>(e.Value)) {
                        const auto parentValue = std::get<std::string_view>(e.Value);

                        ActorID parentId = {};
                        if (parentValue != "root") {
                            parentId = ActorID(parentValue);
                        }

                        m_ParentPairsToResolve.emplace_back(m_CurrentEntityID, parentId);
                    }
                }

                if (m_CurrentEntityID.IsValid() && !m_CurrentEntityName.empty() && !m_CurrentEntityState.empty()) {
                    if (m_CurrentEntityState == "added") {
                        m_CurrentEntity = m_Scene->CreateActor(m_CurrentEntityID, m_CurrentEntityName);
                    }
                    else if (m_CurrentEntityState == "changed") {
                        m_CurrentEntity = m_Scene->GetActor(m_CurrentEntityID);
                    }
                    else if (m_CurrentEntityState == "removed") {
                        m_Scene->DestroyEntity(m_CurrentEntityID);
                        m_SkipToNextSection = true;
                    }
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

    static std::string_view GetStateName(const ActorPatch::State state) {
        switch (state) {
        case ActorPatch::State::Changed: return "changed";
        case ActorPatch::State::Added: return "added";
        case ActorPatch::State::Removed: return "removed";
        }

        return "added";
    }

    std::optional<ActorPatch::State> Collapse(const Deque<ActorPatch::State>& patches) {
        std::optional<ActorPatch::State> lastChange = std::nullopt;

        for (ActorPatch::State state : std::ranges::reverse_view(patches)) {
            if (state == ActorPatch::State::Added || state == ActorPatch::State::Removed) {
                return state;
            }

            if (!lastChange) {
                lastChange = state;
            }
        }

        return lastChange;
    }

    void SceneSerializer::SerializePatches() {
        flecs::world& world = m_Scene->GetWorld();
        ComponentRegistry& registry = m_Scene->GetComponentRegistry();

        std::filesystem::path patchesFolder = m_ScenePath / s_ScenePatchesFolder;
        HashSet<std::string_view> fieldsToWrite;
        for (auto& [actorId, patch] : m_Actors) {
            if (std::optional<ActorPatch::State> patchStateResult = Collapse(patch.PatchStates); !patchStateResult) {
                continue;
            }

            std::string guid = actorId.ToString();

            std::filesystem::path filePath = patchesFolder / (guid + s_ScenePatchFileExtension);
            std::ifstream patchReadFile(filePath, std::ios::binary | std::ios::ate);

            if (patchReadFile) {
                const size_t patchFileSize = patchReadFile.tellg();
                patchReadFile.seekg(0);

                std::string patchFileBuffer;
                patchFileBuffer.resize(patchFileSize);
                patchReadFile.read(patchFileBuffer.data(), patchFileSize);

                m_Reader = QuelReader(patchFileBuffer);

                ComponentID componentID{};
                ActorPatch::State previousPatchState;
                SectionKind sectionKind;
                ParserState parserState;
                std::string_view sectionField;

                for (auto&& parserEvent : m_Reader.Parse()) {
                    std::visit([&]<typename TEvent>(const TEvent& e) {
                        using T = std::decay_t<TEvent>;

                        if constexpr (std::is_same_v<T, SectionEvent>) {
                            parserState = ParserState::InSection;
                            if (e.Name == "actor") {
                                sectionKind = SectionKind::Entity;
                            }
                            else if (e.Name == "scene") {
                                sectionKind = SectionKind::SceneHeader;
                            }
                            else {
                                sectionKind = SectionKind::None;
                            }

                            componentID = {};
                        }
                        else if constexpr (std::is_same_v<T, ComponentEvent>) {
                            parserState |= ParserState::InComponent;
                            componentID = ComponentRegistry::GetComponentID(e.Name);
                        }
                        else if constexpr (std::is_same_v<T, FieldEvent>) {
                            if ((parserState & ParserState::InComponent) == ParserState::None) {
                                sectionField = e.Path;
                                return;
                            }

                            if ((parserState & (ParserState::InArray | ParserState::InTuple)) != ParserState::None) {
                                return;
                            }

                            ++patch.Components[componentID].Fields[e.Path];
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
                                        previousPatchState = ActorPatch::State::Added;
                                    }
                                    else if (state == "removed") {
                                        previousPatchState = ActorPatch::State::Removed;
                                    }
                                    else if (state == "changed") {
                                        previousPatchState = ActorPatch::State::Changed;
                                    }
                                }
                            } else if (sectionField == "parent") {
                                patch.ParentPatchCount++;
                            }
                        }
                    }, parserEvent);
                }

                patch.PushFrontState(previousPatchState);
            }

            std::string stringBuffer;
            StringQuelWriter quelWriter(stringBuffer);
            quelWriter.SetIndent(2);

            const Entity entity = m_Scene->GetActor(actorId);

            ActorPatch::State patchState = Collapse(patch.PatchStates).value();
            quelWriter.Write(SectionEvent{"actor"});
            quelWriter.WriteField("guid", guid);
            quelWriter.WriteField("name", std::string_view(entity.GetName()));
            quelWriter.WriteField("state", GetStateName(patchState));

            quelWriter.CloseSection();

            if (patchState == ActorPatch::State::Removed) {
                continue;
            }

            if (patch.ParentPatchCount > 0) {
                if (const Entity parent = entity.GetParent(); !parent.IsValid()) {
                    quelWriter.WriteField("parent", "root");
                } else {
                    quelWriter.WriteField("parent", parent.Get<ActorTag>().ID.ToString());
                }
            }

            for (auto& [componentId, compPatch] : patch.Components) {
                const SerializableComponentInfo* info = registry.GetSerializableComponentInfo(componentId);
                if (!info) {
                    continue;
                }

                void* ptr = ecs_get_mut_id(world.c_ptr(), entity.GetID(), info->RuntimeID);
                if (!ptr) {
                    continue;
                }

                quelWriter.Write(ComponentEvent{info->Name});

                fieldsToWrite.clear();

                for (auto& [field, changesCount] : compPatch.Fields) {
                    if (changesCount < 1) {
                        return;
                    }

                    fieldsToWrite.emplace(field);
                }

                QuelWriteArchive archive(quelWriter, &fieldsToWrite);
                info->SerializeTextWriteFunc(archive, ptr);
            }

            std::ofstream file(filePath, std::ios::binary);
            if (!file) {
                QS_CORE_ERROR_TAG("SceneSerializer", "Failed to open file: {}", filePath.string());
                continue;
            }

            file.write(
                stringBuffer.data(),
                static_cast<std::streamsize>(stringBuffer.size())
            );
        }

        m_Actors.clear();
    }

    void SceneSerializer::BakePatches() const {
        flecs::world& world = m_Scene->GetWorld();
        ComponentRegistry& registry = m_Scene->GetComponentRegistry();
        const auto& types = registry.GetSerializableComponents();

        std::vector<std::byte> buffer;
        BinaryWriter writer(buffer);

        auto rootActorsQuery = world.query_builder<ActorTag>()
                                    .without(flecs::ChildOf, flecs::Wildcard)
                                    .build();

        // Header
        SceneHeader header{};
        header.ComponentTypeCount = types.size();
        header.EntityCount = rootActorsQuery.count();

        writer.Write(header);

        Vec<ActorID> rootActors;
        rootActors.reserve(header.EntityCount);

        rootActorsQuery.each([&rootActors](const ActorTag& tag) {
            rootActors.emplace_back(tag.ID);
        });

        std::ranges::sort(
            rootActors,
            [](const ActorID& a, const ActorID& b) {
                return a < b;
            }
        );

        for (auto& actorId : rootActors) {
            ActorSnapshot snapshot = ActorSnapshot::Create(m_Scene, actorId);
            writer.Write<uint32_t>(snapshot.Data.size());
            writer.WriteBytes(std::span(snapshot.Data));
        }

        // Disk write
        std::filesystem::path sceneFilePath = m_ScenePath / (m_ScenePath.filename().string() + s_SceneFileExtension);
        std::ofstream sceneFile(sceneFilePath, std::ios::binary);

        if (!sceneFile) {
            QS_ERROR_TAG("SceneSerializer", "Failed to open file '{}'!", sceneFilePath.string());
            return;
        }

        sceneFile.write(
            reinterpret_cast<const char*>(buffer.data()),
            static_cast<std::streamsize>(buffer.size())
        );

        for (auto& patchFileEntry : std::filesystem::directory_iterator(m_ScenePath / s_ScenePatchesFolder)) {
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

            std::filesystem::remove(patchFilePath);
        }
    }
}
