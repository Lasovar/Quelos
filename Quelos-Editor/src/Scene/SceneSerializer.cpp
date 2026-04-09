#include "qspch.h"
#include "SceneSerializer.h"

#include <ranges>
#include <string_view>

using namespace magic_enum::bitwise_operators;

namespace Quelos {
    using namespace Serialization;

    static constexpr std::string_view GetStateName(const PatchState state) {
        switch (state) {
        case PatchState::Changed: return "changed";
        case PatchState::Added: return "added";
        case PatchState::Removed: return "removed";
        }

        return "added";
    }

    static constexpr PatchState GetPatchState(const std::string_view state) {
        if (state == "changed") {
            return PatchState::Changed;
        }

        if (state == "added") {
            return PatchState::Added;
        }

        if (state == "removed") {
            return PatchState::Removed;
        }

        return PatchState::Changed;
    }

    SceneSerializer::SceneSerializer(const Ref<Scene>& scene, const Path& sceneFolderPath)
        : m_Scene(scene), m_ScenePath(Project::GetProjectPath() / sceneFolderPath) { }

    void SceneSerializer::DeserializeComponentData() {
        if (!m_CurrentEntityID.IsValid() && !m_CurrentComponentID.IsValid()) {
            return;
        }

        auto& registry = m_Scene->GetComponentRegistry();
        const SerializableComponentInfo* componentInfo = registry.GetSerializableComponentInfo(m_CurrentComponentID);

        if (!componentInfo) {
            return;
        }

        QuelReadArchive archive(m_FieldTable, m_ValuePool);
        void* data = m_CurrentEntity.GetInternalID().ensure(componentInfo->RuntimeID);

        if (!data) {
            QS_ERROR_TAG("SceneSerializer::DeserializeComponentData", "Failed to ensure component");
            return;
        }

        componentInfo->SerializeTextReadFunc(archive, data);

        m_FieldsMap.clear();
        m_ContainerStack.clear();

        m_CurrentComponentID = {};
    }

    void SceneSerializer::OnEvent(const SectionEvent& e) {
        // Flush any pending component data
        DeserializeComponentData();

        m_ParserState = ParserState::InSection;

        m_CurrentEntity = {};
        m_CurrentEntityState = "";
        m_CurrentParentID = {};
        m_CurrentEntityName = "";
        m_CurrentEntityID = {};
        m_CurrentComponent = "";

        if (e.Name == "scene") {
            m_SectionKind = SectionKind::SceneHeader;
        }
        else if (e.Name == "actor") {
            m_SectionKind = SectionKind::Actor;
        }
        else {
            m_SectionKind = SectionKind::None;
        }
    }

    void SceneSerializer::OnEvent(const ComponentEvent& e) {
        // Flush previous component before starting a new one
        DeserializeComponentData();

        m_ParserState |= ParserState::InComponent;

        m_ValuePool.clear();
        m_FieldTable.clear();
        m_ContainerStack.clear();

        m_ValuePool.reserve(256);
        m_FieldTable.reserve(128);

        m_CurrentComponent = e.Name;
        m_CurrentComponentID = ComponentRegistry::GetComponentID(m_CurrentComponent);
        m_IsFirstComponentField = true;
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
            case SectionKind::Actor:
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
                else if (m_CurrentEntityID) {
                    if (m_CurrentField == "parent") {
                        if (std::holds_alternative<std::string_view>(e.Value)) {
                            const auto parentValue = std::get<std::string_view>(e.Value);

                            ActorID parentId = {};
                            if (parentValue != "root") {
                                parentId = ActorID(parentValue);
                            }

                            m_CurrentParentID = parentId;
                        }
                    }
                    else if (m_CurrentField == "order") {
                        if (const std::string_view* orderStr = std::get_if<std::string_view>(&e.Value)) {
                            const std::string_view hex = orderStr->substr(2);
                            uint64_t order = 0;

                            auto [ptr, errCode] = std::from_chars(
                                hex.data(),
                                hex.data() + hex.size(),
                                order,
                                16
                            );

                            if (errCode == std::errc()) {
                                m_ParentPairsToResolve[m_CurrentParentID].emplace_back(m_CurrentEntity, order);
                            }
                            else {
                                QS_ERROR_TAG(
                                    "SceneSerializer",
                                    "Failed to read child order for actor '{}({})': {}",
                                    m_CurrentEntityName,
                                    m_CurrentEntityID.ToString(),
                                    std::make_error_code(errCode).message()
                                );
                            }
                        }
                    }
                }

                if (!m_CurrentEntity.IsValid() && m_CurrentEntityID.IsValid() && !m_CurrentEntityName.empty() && !
                    m_CurrentEntityState.empty()) {
                    if (m_CurrentEntityState == "added") {
                        m_CurrentEntity = m_Scene->CreateActor(m_CurrentEntityID, m_CurrentEntityName);
                    }
                    else if (m_CurrentEntityState == "changed") {
                        m_CurrentEntity = m_Scene->GetActor(m_CurrentEntityID);
                        m_CurrentEntity.SetName(m_CurrentEntityName);
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
            if (m_IsFirstComponentField) {
                if (m_CurrentField == "state") {
                    const std::string_view state = std::get<std::string_view>(e.Value);
                    if (GetPatchState(state) == PatchState::Removed) {
                        if (m_CurrentComponentID && m_CurrentEntity.IsValid()) {
                            ComponentRegistry& registry = m_Scene->GetComponentRegistry();

                            const auto* info = registry.GetSerializableComponentInfo(m_CurrentComponentID);
                            if (!info) {
                                return;
                            }

                            m_CurrentEntity.Remove(info->RuntimeID);
                            m_CurrentComponentID = {};
                            m_SkipToNextComponent = true;
                        }
                    }
                }

                m_IsFirstComponentField = false;
            }

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

    std::optional<PatchState> CollapsePatchState(const Deque<PatchState>& patches) {
        std::optional<PatchState> lastChange = std::nullopt;

        for (PatchState state : std::views::reverse(patches)) {
            if (state == PatchState::Added || state == PatchState::Removed) {
                return state;
            }

            if (!lastChange) {
                lastChange = state;
            }
        }

        return lastChange;
    }

    void SceneSerializer::Deserialize() {
        if (!std::filesystem::is_directory(m_ScenePath)) {
            if (std::filesystem::exists(m_ScenePath)) {
                QS_ERROR_TAG("SceneSerializer", "Scene folder path is not a directory");
                return;
            }

            std::filesystem::create_directories(m_ScenePath);
        }

        std::filesystem::path patchesFolder = m_ScenePath / ScenePatchesFolder;
        if (!std::filesystem::exists(patchesFolder)) {
            std::filesystem::create_directories(patchesFolder);
        }

        std::string sceneName = m_ScenePath.filename().generic_string();
        std::filesystem::path sceneFilePath = m_ScenePath / (sceneName + SceneFileExtension);

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

        m_Scene->SetName(sceneName);

        const flecs::world& world = m_Scene->GetWorld();
        world.defer_begin();

        for (uint64_t i = 0; i < header.EntityCount; i++) {
            const auto entitySnapshotSize = reader.Read<uint32_t>();
            if (!entitySnapshotSize) {
                QS_ERROR_TAG("SceneSerializer", "Invalid entity snapshot size!");
                continue;
            }

            const auto entitySnapshotBlob = reader.ReadBytes(entitySnapshotSize.value());
            if (entitySnapshotBlob.empty()) {
                QS_ERROR_TAG("SceneSerializer", "Failed to read entity snapshot!");
                continue;
            }

            EntitySnapshot entitySnapshot;
            entitySnapshot.Data.resize(entitySnapshotSize.value());
            std::memcpy(entitySnapshot.Data.data(), entitySnapshotBlob.data(), entitySnapshotBlob.size());

            EntitySnapshot::Load(m_Scene, entitySnapshot);
        }

        world.defer_end();

        world.defer_begin();

        for (auto& patchFileEntry : std::filesystem::directory_iterator(patchesFolder)) {
            const auto& patchFilePath = patchFileEntry.path();
            if (!patchFileEntry.is_regular_file() || patchFilePath.extension() != ScenePatchFileExtension) {
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

                    if (m_SkipToNextComponent) {
                        if constexpr (std::is_same_v<T, ComponentEvent>) {
                            m_SkipToNextComponent = false;
                        }
                        else {
                            return;
                        }
                    }

                    this->OnEvent(e);
                }, parserEvent);
            }

            // Make sure to flush the last component
            DeserializeComponentData();

            m_ParserState &= ~ParserState::InComponent;
        }

        world.defer_end();

        world.defer_begin();

        for (auto& [parentId, childrenEntries] : m_ParentPairsToResolve) {
            const Actor parent = m_Scene->GetActor(parentId);

            for (auto& [child, order] : childrenEntries) {
                if (parentId) {
                    if (parent.IsValid()) {
                        child.SetParent(parent);
                    }
                }
                else {
                    child.RemoveParent();
                }

                child.GetInternalID().set(ChildOrder{order});
            }
        }

        world.defer_end();

        for (auto& parentId : m_ParentPairsToResolve | std::views::keys) {
            const Actor parent = m_Scene->GetActor(parentId);
            parent.IndexChildOrders();
        }

        m_ParentPairsToResolve.clear();
    }

    void SceneSerializer::SerializePatches() {
        flecs::world& world = m_Scene->GetWorld();
        ComponentRegistry& registry = m_Scene->GetComponentRegistry();

        std::filesystem::path patchesFolder = m_ScenePath / ScenePatchesFolder;
        HashSet<std::string_view> fieldsToWrite;
        for (auto& [actorId, patch] : m_Actors) {
            if (!actorId) {
                continue;
            }

            if (std::optional<PatchState> patchStateResult = CollapsePatchState(patch.PatchStates); !patchStateResult) {
                continue;
            }

            std::string guid = actorId.ToFormattedString();

            std::filesystem::path filePath = patchesFolder / (guid + ScenePatchFileExtension);
            std::ifstream patchReadFile(filePath, std::ios::binary | std::ios::ate);

            if (patchReadFile) {
                const size_t patchFileSize = patchReadFile.tellg();
                patchReadFile.seekg(0);

                std::string patchFileBuffer;
                patchFileBuffer.resize(patchFileSize);
                patchReadFile.read(patchFileBuffer.data(), patchFileSize);

                m_Reader = QuelReader(patchFileBuffer);

                ComponentID componentID{};
                PatchState previousPatchState;
                SectionKind sectionKind;
                ParserState parserState;
                std::string_view sectionField;
                bool firstComponentField = true;

                for (auto&& parserEvent : m_Reader.Parse()) {
                    std::visit([&]<typename TEvent>(const TEvent& e) {
                        using T = std::decay_t<TEvent>;

                        if constexpr (std::is_same_v<T, SectionEvent>) {
                            parserState = ParserState::InSection;
                            if (e.Name == "actor") {
                                sectionKind = SectionKind::Actor;
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
                            firstComponentField = true;
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
                                || sectionKind != SectionKind::Actor
                            ) {
                                return;
                            }

                            if ((parserState & ParserState::InComponent) != ParserState::None) {
                                if (firstComponentField && componentID && sectionField == "state") {
                                    if (std::holds_alternative<std::string_view>(e.Value)) {
                                        const std::string_view state = std::get<std::string_view>(e.Value);
                                        patch.Components[componentID].PushFrontState(GetPatchState(state));
                                    }
                                }

                                firstComponentField = false;
                            }
                            else if (sectionField == "state") {
                                if (std::holds_alternative<std::string_view>(e.Value)) {
                                    const std::string_view state = std::get<std::string_view>(e.Value);

                                    if (state == "added") {
                                        previousPatchState = PatchState::Added;
                                    }
                                    else if (state == "removed") {
                                        previousPatchState = PatchState::Removed;
                                    }
                                    else if (state == "changed") {
                                        previousPatchState = PatchState::Changed;
                                    }
                                }
                            }
                            else if (sectionField == "parent") {
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

            const Actor entity = m_Scene->GetActor(actorId);

            PatchState patchState = CollapsePatchState(patch.PatchStates).value();
            quelWriter.Write(SectionEvent{"actor"});
            quelWriter.WriteField("guid", UnquotedString{guid});
            quelWriter.WriteField("name", std::string_view(entity.GetName()));
            quelWriter.WriteField("state", UnquotedString{GetStateName(patchState)});

            quelWriter.CloseSection();

            if (patchState == PatchState::Removed) {
                continue;
            }

            if (patch.ParentPatchCount > 0) {
                if (const Actor parent = entity.GetParent(); !parent.GetActorID()) {
                    quelWriter.WriteField("parent", UnquotedString{"root"});
                }
                else {
                    quelWriter.WriteField("parent", UnquotedString{parent.GetActorID().ToFormattedString()});
                }

                quelWriter.WriteField(
                    "order",
                    UnquotedString{
                        std::format("0x{:016X}", entity.Get<ChildOrder>().Value)
                    }
                );
            }

            for (auto& [componentId, compPatch] : patch.Components) {
                if (compPatch.PatchStates.empty()) {
                    continue;
                }

                const SerializableComponentInfo* info = registry.GetSerializableComponentInfo(componentId);
                if (!info) {
                    continue;
                }

                quelWriter.Write(ComponentEvent{info->Name});

                PatchState compPatchState = CollapsePatchState(compPatch.PatchStates).value();
                if (compPatchState == PatchState::Removed) {
                    quelWriter.WriteField("state", UnquotedString{GetStateName(compPatchState)});
                    continue;
                }

                void* ptr = ecs_get_mut_id(world.c_ptr(), entity.GetInternalID(), info->RuntimeID);
                if (!ptr) {
                    continue;
                }

                if (compPatchState == PatchState::Changed) {
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
                else {
                    QuelWriteArchive archive(quelWriter);
                    info->SerializeTextWriteFunc(archive, ptr);
                }
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

        Entity sceneRoot = m_Scene->GetSceneRoot();

        // Header
        SceneHeader header{};
        header.ComponentTypeCount = types.size();
        header.EntityCount = sceneRoot.ChildrenCount();

        writer.Write(header);

        Vec<ActorID> rootActors;
        rootActors.reserve(header.EntityCount);

        sceneRoot.GetInternalID().children([&rootActors](const flecs::entity child) {
            if (auto* actorTag = child.try_get<ActorTag>()) {
                rootActors.push_back(actorTag->ID);
            }
        });

        for (auto& actorId : rootActors) {
            EntitySnapshot snapshot = EntitySnapshot::Create(m_Scene, actorId);
            writer.Write<uint32_t>(snapshot.Data.size());
            writer.WriteBytes(std::span(snapshot.Data));
        }

        // Disk write
        std::filesystem::path sceneFilePath = m_ScenePath / (m_ScenePath.filename().string() + SceneFileExtension);
        std::ofstream sceneFile(sceneFilePath, std::ios::binary);

        if (!sceneFile) {
            QS_ERROR_TAG("SceneSerializer", "Failed to open file '{}'!", sceneFilePath.string());
            return;
        }

        sceneFile.write(
            reinterpret_cast<const char*>(buffer.data()),
            static_cast<std::streamsize>(buffer.size())
        );

        for (auto& patchFileEntry : std::filesystem::directory_iterator(m_ScenePath / ScenePatchesFolder)) {
            const auto& patchFilePath = patchFileEntry.path();
            if (!patchFileEntry.is_regular_file() || patchFilePath.extension() != ScenePatchFileExtension) {
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
