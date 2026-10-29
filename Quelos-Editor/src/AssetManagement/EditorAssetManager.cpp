#include "qspch.h"
#include "EditorAssetManager.h"

#include "AssetImporters/ModelImporter.h"
#include "AssetImporters/SceneImporter.h"
#include "EditorAssetImporter.h"
#include "ImportContext.hpp"
#include "SidecarHandler.hpp"
#include "xxhash.h"
#include "AssetImporters/MaterialImporter.h"
#include "AssetImporters/ShaderImporter.h"
#include "Quelos/AssetManager/AssetRegistryExtensions.h"
#include "AssetImporters/TextureImporter.h"
#include "Quelos/ImGui/ImGuiUI.h"
#include "Quelos/Project/Project.h"
#include "Quelos/Serialization/Serializer.h"

namespace QuelosEditor {
    static const OsPath k_AssetRegistryFilename = "AssetRegistry.quel";

    AssetID EditorAssetManager::AddAssetToRegistry(const std::string_view assetPath) {
        if (const AssetMetadata* metadata = m_AssetRegistry.GetAssetMetadata(assetPath)) {
            return metadata->Handle;
        }

        OsPath asset(assetPath);
        if (asset.is_absolute()) {
            asset = std::filesystem::relative(asset, Project::GetProjectPath());
        }

        const std::string relativePath = asset.lexically_normal().generic_string();

        const AssetType assetType = AssetImporter::GetAssetType(relativePath);
        if (!assetType) {
            QS_CORE_ERROR_TAG(
                "EditorAssetManager::AddAssetToRegistry",
                "Failed to add asset '{}' to registry: unknown asset type!",
                assetPath
            );

            return {};
        }

        AssetID handle;

        if (const AssetID existingHandle = EditorAssetImporter::ReadAssetHandle(relativePath)) {
            handle = existingHandle;
            QS_CORE_INFO_TAG(
                "EditorAssetManager::AddAssetToRegistry",
                "Using existing asset handle {} for '{}'",
                handle.ToString(),
                assetPath
            );
        }
        else {
            handle = AssetID::Generate();
            QS_CORE_INFO_TAG(
                "EditorAssetManager::AddAssetToRegistry",
                "Generated new asset handle {} for '{}'",
                handle.ToString(),
                assetPath
            );

            EditorAssetImporter::WriteAssetHandle(relativePath, handle);
        }

        const AssetMetadata assetMetadata = {
            handle,
            relativePath,
            assetType
        };


        if (Expected<void, String> r = m_Database.RegisterAsset(
                handle,
                relativePath,
                assetType.GetName(),
                None
            ); !r
        ) {
            QS_ERROR_TAG(
                "EditorAssetManager::AddAssetToRegistry",
                "Failed to register '{}': {}",
                relativePath,
                r.error()
            );

            return {};
        }

        m_AssetRegistry.GetAssetsMetadata()[handle] = assetMetadata;
        return handle;
    }

    Vec<AssetID> EditorAssetManager::ProcessAssetRegistration(
        const std::string_view assetPath, const std::pmr::polymorphic_allocator<> allocator
    ) {
        Vec<AssetID> registeredAssets(allocator);

        AssetID mainAsset = AddAssetToRegistry(assetPath);
        if (!mainAsset) {
            return registeredAssets;
        }

        registeredAssets.push_back(mainAsset);

        const Vec<AssetMetadata> additionalAssets = AssetRegistryExtensions::ProcessAssetRegistration(
            assetPath,
            *GetAssetMetadata(mainAsset)
        );

        Vec<AssetMetadata> subAssets(Allocator::Frame);
        subAssets.reserve(additionalAssets.size());

        for (const auto& additionalAsset : additionalAssets) {
            if (auto r = m_Database.RegisterAsset(
                additionalAsset.Handle,
                additionalAsset.FilePath,
                additionalAsset.Type.GetName(),
                mainAsset
            ); !r) {
                QS_ERROR_TAG(
                    "EditorAssetManager::ProcessAssetRegistration",
                    "Failed to register sub-asset '{}': {}",
                    additionalAsset.FilePath, r.error()
                );

                continue;
            }

            subAssets.push_back(additionalAsset);
            registeredAssets.push_back(additionalAsset.Handle);
        }

        SyncSubAssetsToRegistry(mainAsset, subAssets);
        return registeredAssets;
    }

    Vec<AssetID> EditorAssetManager::ProcessAssetRegistration(
        const std::string_view assetPath,
        const AllocatorType allocatorType
    ) {
        return ProcessAssetRegistration(assetPath, GetAllocator(allocatorType));
    }

    void EditorAssetManager::RemoveAssetFromRegistry(const AssetID& assetHandle) {
        m_AssetRegistry.RemoveAsset(assetHandle);
        m_WatchedAssets.erase(assetHandle);
    }

    void EditorAssetManager::QueueReimport(const AssetID assetId) {
        m_ReimportQueue.emplace(assetId);
    }

    Optional<AssetID> EditorAssetManager::ResolveOwningAsset(const std::string_view absolutePath) const {
        const std::string relative = OsPath(absolutePath).lexically_relative(Project::GetProjectPath()).generic_string();

        std::string_view path = relative;
        if (FS::Extension(path) == ".qmeta") {
            path = FS::RemoveExtension(path);
        }

        // A file inside a directory asset belongs to that directory
        while (!path.empty() && FS::IsInsideDirectory(path, "Assets")) {
            if (const AssetMetadata* meta = GetAssetMetadata(path); meta && !meta->IsSubAsset()) {
                return meta->Handle;
            }

            path = FS::Parent(path);
        }

        return None;
    }

    struct DirectoryEntryStamp {
        String RelativePath;
        int64_t Size = 0;
        int64_t ModTime = 0;
    };

    static Vec<DirectoryEntryStamp> ListDirectory(const OsPath& dir, const Allocator allocator) {
        Vec<DirectoryEntryStamp> entries(allocator);
        std::error_code ec;
        for (auto it = std::filesystem::recursive_directory_iterator(dir, ec);
             it != std::filesystem::recursive_directory_iterator(); it.increment(ec)) {
            if (ec) {
                ec.clear();
                continue;
            }

            if (!it->is_regular_file(ec) || ec) {
                ec.clear();
                continue;
            }

            // TODO: if (IsIgnoredFile(it->path())) continue;   // sidecars, editor temp/swap files, .DS_Store

            const int64_t size = static_cast<int64_t>(it->file_size(ec));
            const int64_t modTime = it->last_write_time(ec).time_since_epoch().count();
            if (ec) {
                ec.clear();
                continue;
            }

            entries.emplace_back(
                String(it->path().lexically_relative(dir).generic_string(), allocator),
                size,
                modTime
            );
        }

        // Directory iteration order is unspecified; sorting makes the hash deterministic
        std::ranges::sort(entries, {}, &DirectoryEntryStamp::RelativePath);
        return entries;
    }

    static uint64_t HashListing(const Span<const DirectoryEntryStamp> entries) {
        const UniquePtr<XXH3_state_t, decltype(&XXH3_freeState)> state(XXH3_createState(), &XXH3_freeState);
        XXH3_64bits_reset(state.get());

        for (const auto& e : entries) {
            XXH3_64bits_update(state.get(), e.RelativePath.data(), e.RelativePath.size() + 1);
            XXH3_64bits_update(state.get(), &e.Size, sizeof(e.Size));
            XXH3_64bits_update(state.get(), &e.ModTime, sizeof(e.ModTime));
        }

        return XXH3_64bits_digest(state.get());
    }

    static Optional<Hash128> HashDirectoryContents(const OsPath& dir, const Span<const DirectoryEntryStamp> entries) {
        const UniquePtr<XXH3_state_t, decltype(&XXH3_freeState)> state(XXH3_createState(), &XXH3_freeState);
        XXH3_128bits_reset(state.get());
        for (const auto& e : entries) {
            XXH3_128bits_update(state.get(), e.RelativePath.data(), e.RelativePath.size() + 1);
            auto fileHash = Hash::File((dir / e.RelativePath.view()).generic_string());
            if (!fileHash) return {};                 // vanished or locked mid-hash
            XXH3_128bits_update(state.get(), &*fileHash, sizeof(*fileHash));
        }

        XXH128_canonical_t canonical;
        XXH128_canonicalFromHash(&canonical, XXH3_128bits_digest(state.get()));

        Hash128 result;
        static_assert(sizeof(result) == sizeof(canonical));
        std::memcpy(&result, &canonical, sizeof(result));

        return { result };
    }

    static Optional<SidecarStamp> StatSidecar(const OsPath& assetPath) {
        const OsPath metaPath = SidecarHandler::GetSidecarPath(assetPath);
        std::error_code ec;
        const auto size = std::filesystem::file_size(metaPath, ec);
        if (ec) {
            return None;
        }

        const auto time = std::filesystem::last_write_time(metaPath, ec);
        if (ec) {
            return None;
        }

        return SidecarStamp{
            .Size = static_cast<int64_t>(size),
            .ModTime = time.time_since_epoch().count(),
        };
    }

    static Optional<ScannedSource> StatSource(
        const OsPath& absolute,
        const std::string_view relativePath,
        const bool usesSidecar,
        const AllocatorType allocator
    ) {
        ScannedSource src{ .Path = String(relativePath, allocator) };

        if (std::error_code ec; std::filesystem::is_directory(absolute, ec)) {
            const auto listing = ListDirectory(absolute, Allocator::Frame);
            src.ModTime = static_cast<int64_t>(HashListing(listing));
            for (const auto& entry : listing) {
                src.Size += entry.Size;
            }
        }
        else {
            src.Size = static_cast<int64_t>(std::filesystem::file_size(absolute, ec));
            if (ec) {
                return None;
            }

            src.ModTime = std::filesystem::last_write_time(absolute, ec).time_since_epoch().count();
            if (ec) {
                return None;
            }
        }

        if (usesSidecar) {
            src.Meta = StatSidecar(absolute);
        }

        return src;
    }

    void EditorAssetManager::FlushReimportQueue() {
        if (m_ReimportQueue.empty()) [[likely]] {
            return;
        }

        ReimportWithDependents(m_ReimportQueue);
        m_ReimportQueue.clear();
    }

    void EditorAssetManager::ProcessFileEvents(float deltaTime) {
        if (m_HasFileEvents.exchange(false, std::memory_order_acquire)) {
            m_AccumulateEventsTimer = 0.3f;
            m_RequestProcessFileEvents = true;
            return;
        }

        if (m_AccumulateEventsTimer > 0.0f) {
            m_AccumulateEventsTimer -= deltaTime;
            return;
        }

        if (!m_RequestProcessFileEvents) {
            return;
        }

        m_RequestProcessFileEvents = true;

        m_ProcessingPaths.clear();

        bool fullRefresh;
        {
            std::scoped_lock lock(m_FileEventsMutex);
            m_ProcessingPaths.swap(m_ModifiedPaths);
            fullRefresh = std::exchange(m_RequestFullRefresh, false);
        }

        if (fullRefresh) {
            // TODO: Might be worth thinking about a more optimized version
            // Instead of refreshing the whole asset tree
            // Like tracking the moved/deleted/added files only, through file watcher events
            RefreshAssetDatabase();   // covers the modified files too
            return;
        }

        HashSet<AssetID> checked(Allocator::Frame); // an asset can get several events per batch (source + .qmeta)
        HashSet<AssetID> changed(Allocator::Frame);
        Vec<SourceStamp> stampUpdates(Allocator::Frame);

        for (const std::string& path : m_ProcessingPaths) {
            const Optional<AssetID> owner = ResolveOwningAsset(path);
            if (!owner || !checked.insert(*owner).second) {
                continue;
            }

            const AssetMetadata* meta = GetAssetMetadata(*owner);
            const OsPath absolute = Project::GetProjectPath() / std::string_view(meta->FilePath);
            const Optional<ScannedSource> src =
                StatSource(absolute, meta->FilePath, EditorAssetImporter::IsUsingSidecar(meta->Type), Allocator::Frame);
            if (!src) {
                continue;   // gone or locked mid-save; a later event or the refresh handles it
            }

            const auto record = m_Database.LoadSourceRecord(*owner, GetAllocator(Allocator::Frame));
            if (!record || !*record) {
                if (!record) {
                    QS_ERROR_TAGGED("Failed to load source record: {}", record.error());
                }

                changed.insert(*owner);   // never imported, or unreadable: import to be safe
                continue;
            }

            switch (CheckSource(**record, *src)) {
            case SourceCheck::Unchanged:
                break;
            case SourceCheck::StampOnly:
                stampUpdates.emplace_back(
                    *owner,
                    src->Size,
                    src->ModTime,
                    src->Meta ? Optional(src->Meta->Size) : None,
                    src->Meta ? Optional(src->Meta->ModTime) : None
                );
                break;
            case SourceCheck::NeedsReimport:
                changed.insert(*owner);
                break;
            }
        }

        if (!stampUpdates.empty()) {
            if (auto r = m_Database.UpdateSourceStamps(stampUpdates); !r) {
                QS_WARN_TAGGED("Failed to update source stamps: {}", r.error());
            }
        }

        ReimportWithDependents(changed);
    }

    bool EditorAssetManager::Import(
        const UntypedAssetPool& pool, UntypedAssetHandle assetHandle, const AssetMetadata& metadata
    ) {
        const AssetID assetId = metadata.Handle;
        void* slotData = pool.GetSlotData(pool.Data, assetHandle);

        if (metadata.IsSubAsset()) {
            if (AssetRegistryExtensions::ResolveSubAsset(slotData, metadata)) {
                pool.SetConstructed(pool.Data, assetHandle, true);
                m_LoadedAssets[assetId] = assetHandle;
                return true;
            }

            QS_CORE_ERROR_TAG(
                "EditorAssetManager::Acquire",
                "Failed to resolve sub-asset {} with parent {}",
                assetId.ToString(),
                metadata.ParentId.ToString()
            );

            pool.DestroyAt(pool.Data, assetHandle);
            return false;
        }


        Optional<SidecarReader> sidecarReader;
        if (EditorAssetImporter::IsUsingSidecar(metadata.Type)) {
            auto sidecarResult= SidecarHandler::ReadSidecar(
                metadata.Type.GetName(),
                metadata.FilePath
            );

            if (sidecarResult) {
                sidecarReader = std::move(*sidecarResult);
            } else {
                QS_ERROR_TAGGED(
                    "Failed to read sidecar for asset '{}': {}",
                    metadata.FilePath, sidecarResult.error()
                );
            }
        }

        LoadContext loadContext = sidecarReader
                                      ? LoadContext(metadata, sidecarReader->GetReadArchive(), slotData)
                                      : LoadContext(metadata, slotData);

        if (!EditorAssetImporter::LoadAsset(loadContext)) {
            QS_CORE_ERROR_TAG(
                "EditorAssetManager::Acquire",
                "Failed to load asset with handle {}, path: '{}'",
                assetId.ToString(),
                metadata.FilePath
            );

            pool.DestroyAt(pool.Data, assetHandle);
            return false;
        }

        pool.SetConstructed(pool.Data, assetHandle, true);
        m_LoadedAssets[assetId] = assetHandle;
        return true;
    }

    UntypedAssetHandle EditorAssetManager::Acquire(const AssetID assetId) {
        if (!IsAssetHandleValid(assetId)) {
            return {};
        }

        if (const auto it = m_LoadedAssets.find(assetId); it != m_LoadedAssets.end()) {
            if (IsAlive(it->second)) {
                return it->second;
            }

            m_LoadedAssets.erase(it);
        }

        const AssetMetadata& metadata = *m_AssetRegistry.GetAssetMetadata(assetId);

        const auto it = m_AssetPools.find(metadata.Type);
        if (it == m_AssetPools.end()) {
            QS_ERROR_TAG(
                "EditorAssetManager::Acquire",
                "No suitable asset pool found for asset type '{}'!",
                metadata.Type.GetName()
            );

            return {};
        }

        const auto pool = it->second;
        const UntypedAssetHandle assetHandle = pool.Allocate(pool.Data);
        if (!Import(pool, assetHandle, metadata)) {
            return {};
        }

        return assetHandle;
    }

    void EditorAssetManager::Release(const UntypedAssetHandle assetHandle) {
        const auto it = m_AssetPools.find(assetHandle.Type);
        if (it == m_AssetPools.end()) {
            return;
        }

        const auto pool = it->second;
        pool.DestroyAt(pool.Data, assetHandle);
    }

    void EditorAssetManager::Release(const AssetID assetId) {
        const auto it = m_LoadedAssets.find(assetId);
        if (it == m_LoadedAssets.end()) {
            return;
        }

        Release(it->second);
        m_LoadedAssets.erase(it);
    }

    bool EditorAssetManager::IsAlive(const UntypedAssetHandle handle) {
        const auto it = m_AssetPools.find(handle.Type);
        if (it == m_AssetPools.end()) {
            return false;
        }

        const auto& pool = it->second;
        return pool.IsValid(pool.Data, handle);
    }

    void EditorAssetManager::IncRef(const UntypedAssetHandle handle) {
        const auto it = m_AssetPools.find(handle.Type);
        if (it == m_AssetPools.end()) {
            return;
        }

        const auto& pool = it->second;
        pool.IncRef(pool.Data, handle);
    }

    void EditorAssetManager::DecRef(const UntypedAssetHandle handle) {
        const auto it = m_AssetPools.find(handle.Type);
        if (it == m_AssetPools.end()) {
            return;
        }

        const auto& pool = it->second;
        pool.DecRef(pool.Data, handle);
    }

    void EditorAssetManager::CleanupAssetMap() {
        std::erase_if(m_LoadedAssets, [&] (const auto& it){ return !IsAlive(it.second); });
    }

    bool EditorAssetManager::IsAssetHandleValid(const AssetID& handle) const {
        if (!handle) {
            return false;
        }

        return m_AssetRegistry.IsAssetHandleValid(handle);
    }

    bool EditorAssetManager::IsAssetPathValid(const std::string_view path) const {
        return m_AssetRegistry.IsAssetPathValid(path);
    }

    Vec<const AssetMetadata*> EditorAssetManager::FindAssetsOfType(
        const AssetTypeID type, const AllocatorType allocatorType
    ) const {
        return FindAssetsOfType(type, GetAllocator(allocatorType));
    }

    Vec<const AssetMetadata*> EditorAssetManager::FindAssetsOfType(
        const AssetTypeID type, const std::pmr::polymorphic_allocator<> allocator
    ) const {
        Vec<const AssetMetadata*> results(allocator);

        const HashMap<AssetID, AssetMetadata>& assetsMetadata = m_AssetRegistry.GetAssetsMetadata();
        for (const AssetMetadata& metadata : assetsMetadata | std::views::values) {
            if (metadata.Type == type) {
                results.push_back(&metadata);
            }
        }

        return results;
    }

    Expected<void, String> EditorAssetManager::RenameAsset(const AssetID& handle, const std::string_view newPath) {
        const AssetMetadata* meta = m_AssetRegistry.GetAssetMetadata(handle);
        if (!meta) {
            return Unexpected(String("Unknown asset", Allocator::Persistent));
        }

        if (meta->IsSubAsset()) {
            return Unexpected(String("Sub-assets are renamed in their source file", Allocator::Persistent));
        }

        const String normalized(OsPath(newPath).lexically_normal().generic_string(), Allocator::Frame);
        if (!FS::IsInsideDirectory(normalized, "Assets")) {
            return Unexpected(String("Target is outside the assets directory", Allocator::Persistent));
        }

        const OsPath& root = Project::GetProjectPath();
        const OsPath oldPath = root / meta->FilePath;
        const OsPath target = root / normalized.view();

        std::error_code ec;
        // A case-only rename on a case-insensitive filesystem points at the same file; allow it
        if (std::filesystem::exists(target, ec) && !std::filesystem::equivalent(oldPath, target, ec)) {
            return Unexpected(String("An asset already exists at that path", Allocator::Persistent));
        }

        std::filesystem::create_directories(target.parent_path(), ec);
        std::filesystem::rename(oldPath, target, ec);
        if (ec) {
            return Unexpected(String(ec.message(), Allocator::Persistent));
        }

        if (EditorAssetImporter::IsUsingSidecar(meta->Type)) {
            std::filesystem::rename(SidecarHandler::GetSidecarPath(oldPath), SidecarHandler::GetSidecarPath(target), ec);
            if (ec) {
                std::error_code undo;
                std::filesystem::rename(target, oldPath, undo);   // put the file back so the two stay together
                return Unexpected(String(ec.message(), Allocator::Persistent));
            }
        }

        return OnAssetMoved(handle, normalized);
    }

    // Bookkeeping only: the file is already at newPath. Safe to call more than once.
    Expected<void, String> EditorAssetManager::OnAssetMoved(const AssetID& handle, const std::string_view newPath) {
        auto& assets = m_AssetRegistry.GetAssetsMetadata();
        const auto it = assets.find(handle);
        if (it == assets.end()) {
            return Unexpected(String("Unknown asset", Allocator::Persistent));
        }

        if (it->second.FilePath == newPath) {
            return {}; // already recorded
        }

        const String oldPath(it->second.FilePath, Allocator::Frame);

        // Database first: parent and children's virtual paths
        if (auto r = m_Database.MoveAsset(handle, oldPath, newPath); !r) {
            return r;
        }

        // registry
        it->second.FilePath = newPath;
        for (auto& child : assets | std::views::values) {
            if (child.ParentId != handle) {
                continue;
            }

            const std::string_view suffix = std::string_view(child.FilePath).substr(oldPath.size()); // "/Meshes/Cube"
            String updated(Allocator::Frame);
            updated.append(newPath).append(suffix);
            child.FilePath = updated;
        }

        return {};
    }

    bool EditorAssetManager::IsAssetSupported(const std::string_view path) {
        return AssetImporter::IsAssetSupported(path);
    }

    bool EditorAssetManager::IsAssetLoaded(const AssetID& handle) const {
        if (!handle) {
            return false;
        }

        return m_LoadedAssets.find(handle) != m_LoadedAssets.end();
    }

    EditorAssetManager::EditorAssetManager() {
        ModelImporter::Initialize();
        RegisterType<Model>();
        RegisterType<Mesh>();

        ShaderImporter::Initialize();
        RegisterType<GraphicsShader>();

        MaterialImporter::Initialize();
        RegisterType<Material>();

        AssetImporter::RegisterAssetImporter(SceneImporter::GetImporterConfig());

        EditorAssetImporter::RegisterAssetImporter(TextureImporter::GetImporterConfig());
        RegisterType<Texture>();
    }

    void EditorAssetManager::Initialize() {
        if (!LoadAssetRegistry()) {
            // Database unreadable; the refresh below rebuilds what it can from sidecars
        }

        RefreshAssetDatabase();

        m_WatchID = m_FileWatcher.addWatch(
            (Project::GetAssetsPath()).generic_string(),
            this,
            true
        );

        m_FileWatcher.watch();
    }

    void EditorAssetManager::UnloadAsset(const AssetID assetHandle) {
        if (!IsAssetHandleValid(assetHandle)) {
            return;
        }

        m_LoadedAssets.erase(assetHandle);
    }

    const AssetMetadata* EditorAssetManager::GetAssetMetadata(const std::string_view path) const {
        if (!IsAssetPathValid(path)) {
            return nullptr;
        }

        return m_AssetRegistry.GetAssetMetadata(path);
    }

    const AssetMetadata* EditorAssetManager::GetAssetMetadata(const AssetID& assetHandle) const {
        return m_AssetRegistry.GetAssetMetadata(assetHandle);
    }

    bool EditorAssetManager::LoadAssetRegistry() {
        constexpr auto Tag = "EditorAssetManager::LoadAssetRegistry";

        auto rows = m_Database.LoadAssets(Allocator::Frame);
        if (!rows) {
            QS_ERROR_TAG(Tag, "Failed to load assets from the database: {}", rows.error());
            return false;
        }

        auto& assets = m_AssetRegistry.GetAssetsMetadata();
        assets.clear();
        assets.reserve(rows->size());

        for (const AssetMetadata& row : *rows) {
            assets[row.Handle] = row;
        }

        QS_INFO_TAG(Tag, "Loaded {} assets from the database!", assets.size());

        return true;
    }

    void EditorAssetManager::handleFileAction(
        const efsw::WatchID watchId, const std::string& dir,
        const std::string& filename, const efsw::Action action,
        std::string oldFilename
    ) {
        std::string absolute = (OsPath(dir) / filename).lexically_normal().generic_string();

        {
            std::scoped_lock lock(m_FileEventsMutex);
            switch (action) {
            case efsw::Action::Modified: {
                m_ModifiedPaths.push_back(std::move(absolute));
                break;
            }
            case efsw::Action::Add:
            case efsw::Action::Delete:
            case efsw::Action::Moved:
                m_RequestFullRefresh = true;
                break;
            default:
                break;
            }
        }

        m_HasFileEvents.store(true, std::memory_order_release);
    }

    struct DependencyGraph {
        Vec<AssetID> Nodes; // index -> ID
        Vec<uint32_t> InDegree; // per node
        Vec<uint32_t> Offsets; // size N+1; dependents of i are [Offsets[i], Offsets[i+1])
        Vec<uint32_t> Dependents; // size E, node indices
    };

    static DependencyGraph BuildGraph(
        const Span<const GUID128> nodes,
        const Span<const Pair<AssetID, AssetID>> edges,
        std::pmr::memory_resource* memoryResource
    ) {
        DependencyGraph graph{
            .Nodes = Vec(nodes.begin(), nodes.end(), memoryResource),
            .InDegree = Vec<uint32_t>(nodes.size(), 0, memoryResource),
            .Offsets = Vec<uint32_t>(nodes.size() + 1, 0, memoryResource),
            .Dependents = Vec<uint32_t>(memoryResource)
        };

        HashMap<AssetID, uint32_t> index(Allocator::Frame);
        index.reserve(nodes.size());
        for (uint32_t i = 0; i < nodes.size(); i++) {
            index.emplace(nodes[i], i);
        }

        // Convert edges to indices once
        Vec<Pair<uint32_t, uint32_t>> e(Allocator::Frame); // (dependsOn, asset)
        e.reserve(edges.size());
        for (const auto& [asset, dependsOn] : edges) {
            if (asset == dependsOn) {
                continue;
            }

            e.emplace_back(index.at(dependsOn), index.at(asset));
        }

        // Counting pass: how many dependents per node, and in-degrees
        for (auto [from, to] : e) {
            ++graph.Offsets[from + 1];
            ++graph.InDegree[to];
        }

        for (size_t i = 1; i < graph.Offsets.size(); ++i) {
            graph.Offsets[i] += graph.Offsets[i - 1];
        }

        // Fill pass
        graph.Dependents.resize(e.size());
        Vec<uint32_t> cursor(graph.Offsets.begin(), graph.Offsets.end() - 1, Allocator::Frame);
        for (auto [from, to] : e) {
            graph.Dependents[cursor[from]++] = to;
        }

        return graph;
    }

    struct TopoSortResult {
        Vec<uint32_t> Order; // node indices, dependency order
        Vec<uint32_t> Cyclic; // node indices that couldn't be ordered
    };

    static TopoSortResult TopologicalSort(const DependencyGraph& graph, std::pmr::memory_resource* memoryResource) {
        const uint32_t n = graph.Nodes.size();

        Vec<uint32_t> remaining(graph.InDegree.begin(), graph.InDegree.end(), Allocator::Frame);
        Vec<uint32_t> ready(Allocator::Frame);
        ready.reserve(n);

        for (uint32_t i = 0; i < n; i++) {
            if (remaining[i] == 0) {
                ready.push_back(i);
            }
        }

        TopoSortResult result{
            .Order = Vec<uint32_t>(memoryResource),
            .Cyclic = Vec<uint32_t>(memoryResource)
        };

        result.Order.reserve(n);

        while (!ready.empty()) {
            const uint32_t current = ready.back();
            ready.pop_back();
            result.Order.push_back(current);

            for (uint32_t k = graph.Offsets[current]; k < graph.Offsets[current + 1]; k++) {
                const uint32_t dep = graph.Dependents[k];
                if (--remaining[dep] == 0) {
                    ready.push_back(dep);
                }
            }
        }

        if (result.Order.size() != n) {
            for (uint32_t i = 0; i < n; i++) {
                if (remaining[i] <= 0) {
                    continue;
                }

                result.Cyclic.push_back(i);
            }
        }

        return result;
    }

    void EditorAssetManager::SyncSubAssetsToRegistry(const AssetID& parentId, const Span<const AssetMetadata> produced) {
        auto& assets = m_AssetRegistry.GetAssetsMetadata();

        // Existing children of this parent
        HashSet<AssetID> stale(Allocator::Frame);

        for (const auto& [id, meta] : assets) {
            if (meta.ParentId == parentId) {
                stale.insert(id);
            }
        }

        for (const AssetMetadata& sub : produced) {
            stale.erase(sub.Handle);
            assets[sub.Handle] = sub;
        }

        // Children no longer produced by the source
        for (const AssetID& id : stale) {
            if (m_LoadedAssets.contains(id)) {
                QS_WARN_TAG(
                    "EditorAssetManager::SyncSubAssetsToRegistry",
                    "Sub-asset {} was removed from its source but is still loaded",
                    id.ToString(Allocator::Frame)
                );
            }

            RemoveAssetFromRegistry(id);
        }
    }

    void EditorAssetManager::ReimportWithDependents(const HashSet<AssetID>& changed) {
        static constexpr auto Tag = QS_FUNCTION_TAG;

        if (changed.empty()) {
            return;
        }

        auto GetRootAsset = [&](const AssetID id) -> AssetID {
            auto* metadata = GetAssetMetadata(id);
            if (!metadata) {
                return {};
            }

            return metadata->ParentId ? metadata->ParentId : id;
        };

        HashSet<AssetID> withDependents = changed.clone(Allocator::Frame);
        for (Vec<AssetID> dependents(Allocator::Frame); const AssetID& id : changed) {
            dependents.clear();
            if (auto r = m_Database.GetTransitiveDependents(id, dependents); !r) {
                QS_ERROR_TAG(
                    Tag,
                    "Failed to find dependents of '{}', they may be stale: {}",
                    id.ToString(),
                    r.error()
                );

                continue;
            }

            for (const AssetID& dep : dependents) {
                withDependents.insert(GetRootAsset(dep));
            }
        }

        Vec nodes(withDependents.begin(), withDependents.end(), Allocator::Frame);
        auto edges = m_Database.GetDependencyEdges(nodes, Allocator::Frame);
        if (!edges) {
            QS_ERROR_TAG(
                Tag,
                "Failed to load dependency edges: {}",
                edges.error()
            );

            return;
        }

        HashSet<Pair<AssetID, AssetID>> edgeSet(edges->begin(), edges->end(), Allocator::Frame);
        for (auto& id : changed) {
            auto* metadata = GetAssetMetadata(id);
            if (!metadata) {
                continue;
            }

            for (const AssetDependency& dependency : EditorAssetImporter::GetAssetDependencies(
                     *metadata,
                     GetAllocator(Allocator::Frame))) {
                AssetID assetId = std::visit(
                    Overloaded{
                        [&](const AssetID& dependencyId) -> AssetID {
                            return dependencyId;
                        },
                        [&](const String& path) -> AssetID {
                            auto* md = GetAssetMetadata(path);
                            if (!md) {
                                return {};
                            }

                            return md->Handle;
                        }
                    },
                    dependency
                );

                if (!assetId) {
                    continue;
                }

                AssetID target = GetRootAsset(assetId);

                if (target == id) {
                    continue;
                }

                if (!withDependents.contains(target)) {
                    continue; // not being reimported
                }

                if (edgeSet.insert({id, target}).second) {
                    edges->emplace_back(id, target);
                }
            }
        }

        DependencyGraph graph = BuildGraph(nodes, *edges, GetAllocator(Allocator::Frame));

        TopoSortResult sorted = TopologicalSort(graph, GetAllocator(Allocator::Frame));
        for (const auto cyclicIdIndex : sorted.Cyclic) {
            QS_ERROR_TAG(
                Tag,
                "Cyclic asset({}) dependency detected! can't reimport, skipping...",
                graph.Nodes[cyclicIdIndex].ToString()
            );
        }

        static constexpr auto DependentsOf = [](const DependencyGraph& graph, uint32_t node) -> Span<const uint32_t> {
            const uint32_t begin = graph.Offsets[node];
            const uint32_t end   = graph.Offsets[node + 1];
            return { graph.Dependents.data() + begin, end - begin };
        };

        // Skip anything downstream of a failed import
        Vec<bool> failed(graph.Nodes.size(), false, Allocator::Frame);
        for (uint32_t i : sorted.Order) {
            if (failed[i]) {
                QS_WARN_TAG(Tag, "Skipping '{}': a dependency failed to import", graph.Nodes[i].ToString());
            }
            else if (Reimport(graph.Nodes[i])) {
                continue;
            }

            for (uint32_t d : DependentsOf(graph, i)) {
                failed[d] = true;
            }
        }
    }

    void EditorAssetManager::RefreshAssetDatabase() {
        static constexpr auto Tag = QS_FUNCTION_TAG;
        QS_PROFILE_SCOPED();

        // path -> {ID, Size, ModTime, Hash, ImporterVersion, ...}
        auto knownResult = m_Database.LoadSourceRecords(Allocator::Frame);
        if (!knownResult) {
            QS_ERROR_TAG(Tag, "Unable to load existing database records: {}", knownResult.error());
            QS_ASSERT(false);
            return;
        }

        auto& known = *knownResult;
        struct KnownAsset {
            std::string_view Path;
            const SourceRecord* Record;
        };

        HashMap<AssetID, KnownAsset> knownByID(Allocator::Frame);
        knownByID.reserve(known.size());
        for (const auto& [path, rec] : known) {
            knownByID.emplace(rec.ID, KnownAsset{ .Path = path.view(), .Record = &rec });
        }

        HashSet<AssetID> toImport(Allocator::Frame);
        HashSet<AssetID> seenIDs(Allocator::Frame);
        Vec<SourceStamp> stampUpdates(Allocator::Frame);

        const OsPath& assetsDirectory = Project::GetAssetsPath();
        std::error_code ec;
        for (auto it = std::filesystem::recursive_directory_iterator(assetsDirectory, ec);
             it != std::filesystem::recursive_directory_iterator();
             it.increment(ec)
        ) {
            if (ec) {
                QS_ERROR_TAG(
                    Tag,
                    "Failed to recurse through the assets directory: {}",
                    ec.message()
                );

                continue;
            }

            const OsPath& absoluteEntryPath = it->path();

            const bool isDirectory = it->is_directory(ec);
            if (ec) {
                ec.clear();
                continue;
            }

            if (!IsAssetSupported(absoluteEntryPath.generic_string())) {
                continue;
            }

            if (isDirectory) {
                it.disable_recursion_pending(); // the directory asset owns everything inside it
            }

            String relativeAssetPath(
                absoluteEntryPath.lexically_relative(Project::GetProjectPath()).generic_string(),
                Allocator::Frame
            );

            ScannedSource src{ .Path = String(relativeAssetPath, Allocator::Frame) };
            if (isDirectory) {
                auto listing = ListDirectory(absoluteEntryPath, Allocator::Frame);
                src.ModTime = static_cast<int64_t>(HashListing(listing));
                for (const auto& entry : listing) {
                    src.Size += entry.Size;
                }
            }
            else {
                src.Size = static_cast<int64_t>(it->file_size(ec));
                src.ModTime = it->last_write_time(ec).time_since_epoch().count();

                if (ec) {
                    ec.clear();
                    continue;
                }
            }
            const AssetType& assetType = EditorAssetImporter::GetAssetType(relativeAssetPath);
            const bool isUsingSidecar = EditorAssetImporter::IsUsingSidecar(assetType);
            if (isUsingSidecar) {
                src.Meta = StatSidecar(absoluteEntryPath);
            }

            if (auto found = known.find(relativeAssetPath); found != known.end()) {
                auto& record = found->second;
                seenIDs.insert(record.ID);

                if (isUsingSidecar && !src.Meta) {
                    if (auto r = SidecarHandler::WriteAssetID(assetType.GetName(), relativeAssetPath, record.ID)) {
                        QS_WARN_TAGGED("Restored missing '.qmeta' file for '{}', settings might have been lost!", relativeAssetPath);
                    } else {
                        QS_ERROR_TAGGED("Failed to restore meta file for '{}': {}", relativeAssetPath, r.error());
                    }
                }

                switch (CheckSource(record, src)) {
                case SourceCheck::Unchanged:
                    break;
                case SourceCheck::StampOnly:
                    stampUpdates.emplace_back(
                        record.ID,
                        src.Size,
                        src.ModTime,
                        src.Meta ? Optional(src.Meta->Size) : None,
                        src.Meta ? Optional(src.Meta->ModTime) : None
                    );
                    break;
                case SourceCheck::NeedsReimport:
                    toImport.insert(record.ID);
                    break;
                }

                continue;
            }

            if (AssetID existing = EditorAssetImporter::ReadAssetHandle(relativeAssetPath)) {
                if (auto knownIt = knownByID.find(existing); knownIt != knownByID.end()) {
                    std::error_code existsEc;
                    const bool oldPathStillExists = std::filesystem::exists(
                        Project::GetProjectPath() / knownIt->second.Path,
                        existsEc
                    ) || existsEc;

                    if (!oldPathStillExists && !seenIDs.contains(existing)) {
                        // A real move: the file is gone from its old path and nothing else has claimed the ID
                        seenIDs.insert(existing);

                        if (auto r = OnAssetMoved(existing, relativeAssetPath); !r) {
                            QS_ERROR_TAG(Tag, "Failed to record move of '{}': {}", relativeAssetPath, r.error());
                        }

                        switch (CheckSource(*knownIt->second.Record, src)) {
                        case SourceCheck::Unchanged:
                            break;
                        case SourceCheck::StampOnly:
                            stampUpdates.emplace_back(
                                existing,
                                src.Size,
                                src.ModTime,
                                src.Meta ? Optional(src.Meta->Size) : None,
                                src.Meta ? Optional(src.Meta->ModTime) : None
                            );
                            break;
                        case SourceCheck::NeedsReimport:
                            toImport.insert(existing);
                            break;
                        }

                        continue;
                    }

                    // A copy: the original still owns this ID. Give the copy a new one.
                    QS_TRACE_TAG(Tag, "'{}' is a copy of '{}', assigning a new ID", relativeAssetPath, knownIt->second.Path);
                    EditorAssetImporter::WriteAssetHandle(relativeAssetPath, AssetID::Generate());
                    // fall through to registration as a new asset
                }
            }

            Vec<AssetID> registered = ProcessAssetRegistration(relativeAssetPath, Allocator::Frame);
            if (registered.empty()) {
                continue;
            }

            // main asset only, sub-assets come from its import
            seenIDs.insert(registered.front());
            toImport.insert(registered.front());
        }

        for (const auto& rec : known | std::views::values) {
            if (!seenIDs.contains(rec.ID)) {
                // TODO: HandleMissingSource(rec);
            }
        }

        if (!stampUpdates.empty()) {
            if (auto r = m_Database.UpdateSourceStamps(stampUpdates); !r) {
                QS_WARN_TAG(Tag, "Failed to update source stamps: {}", r.error()); // harmless, rehashed next time
            }
        }

        if (auto artifacts = m_Database.LoadAllArtifacts(Allocator::Frame); artifacts) {
            const OsPath& library = Project::GetLibraryPath();
            for (const auto& [owner, path] : *artifacts) {
                if (toImport.contains(owner) || !seenIDs.contains(owner)) {
                    continue;
                }

                ec.clear();
                if (!std::filesystem::exists(library / path.view(), ec) || ec) {
                    QS_TRACE_TAG(Tag, "Artifact '{}' is missing, reimporting its asset", path);
                    toImport.insert(owner);
                }
            }
        }
        else {
            QS_WARN_TAG(Tag, "Couldn't check artifacts: {}", artifacts.error());
        }

        ReimportWithDependents(toImport);
    }

    SourceCheck EditorAssetManager::CheckSource(const SourceRecord& rec, const ScannedSource& src) {
        if (rec.ImporterVersion != AssetImporter::GetVersion(Hash::Fnv1a32(rec.TypeName))) {
            return SourceCheck::NeedsReimport;
        }

        Optional<OsPath> absolutePath;

        SourceCheck sidecarCheck = SourceCheck::Unchanged;
        if (src.Meta) {
            auto& meta = *src.Meta;
            if (rec.MetaModTime != meta.ModTime || rec.MetaSize != meta.Size) {
                absolutePath = Project::GetProjectPath() / src.Path.view();

                const auto hashResult = Hash::File(SidecarHandler::GetSidecarPath(*absolutePath).generic_string());
                if (hashResult != rec.MetaHash) {
                    return SourceCheck::NeedsReimport;
                }

                sidecarCheck = SourceCheck::StampOnly;
            }
        }

        if (rec.Size == src.Size && rec.ModTime == src.ModTime) {
            return sidecarCheck;
        }

        // Metadata changed: confirm with the content hash
        Optional<Hash128> hash = None;

        if (!absolutePath) {
            absolutePath = Project::GetProjectPath() / src.Path.view();
        }

        if (std::filesystem::is_directory(*absolutePath)) {
            auto listing = ListDirectory(*absolutePath, Allocator::Frame);
            hash = HashDirectoryContents(*absolutePath, listing);
        } else {
            hash = Hash::File(absolutePath->generic_string());
        }

        if (hash == rec.Hash) {
            /*
            if (Expected<void, String> r = m_Database.UpdateSourceStamp(rec.ID, src.Size, src.ModTime); !r) {
                QS_CORE_ERROR_TAG("EditorAssetManager::NeedsReimport", "Failed to update source stamp: {}", r.error());
            }
            */

            return SourceCheck::StampOnly;
        }

        return SourceCheck::NeedsReimport;
    }

    bool EditorAssetManager::Reload(const AssetID& assetId) {
        const AssetMetadata* metadata = GetAssetMetadata(assetId);
        if (!metadata) [[unlikely]] {
            QS_CORE_ERROR_TAG(
                "EditorAssetManager",
                "Failed to reload asset! couldn't find asset metadata with AssetID {}",
                assetId.ToString()
            );

            return false;
        }

        const auto assetLoaded = m_LoadedAssets.find(assetId);
        if (assetLoaded != m_LoadedAssets.end()) {
            const auto assetHandle = assetLoaded->second;
            const auto poolIt = m_AssetPools.find(assetHandle.Type);
            if (poolIt == m_AssetPools.end()) {
                return false;
            }

            if (const auto pool = poolIt->second; pool.IsValid(pool.Data, assetHandle)) {
                pool.ReleaseAt(pool.Data, assetHandle);
                QS_TRACE_TAGGED("Asset (id: {}, path: {}) reloaded", assetId.ToFormattedString(), metadata->FilePath);
                if (!Import(pool, assetHandle, *metadata)) {
                    return false;
                }

                if (const auto it = m_AssetReloadEventListeners.find(assetId); it != m_AssetReloadEventListeners.end()) {
                    for (auto handle : it->second.Foreach()) {
                        std::invoke(handle.get());
                    }
                }

                return true;
            }
        }

        return false;
    }

    static Vec<String> ArtifactsNotIn(const Span<const String> previous, const Span<const String> current, const Allocator allocator) {
        const HashSet<std::string_view> keep(current.begin(), current.end(), Allocator::Frame);

        Vec<String> stale(allocator);
        for (const String& path : previous) {
            if (!keep.contains(path)) {
                stale.push_back(path);
            }
        }

        return stale;
    }

    static void DeleteArtifacts(const Span<const String> artifacts) {
        std::error_code ec;
        for (const auto& artifact : artifacts) {
            ec.clear();
            const OsPath& root = Project::GetLibraryPath();
            std::filesystem::remove(root / artifact.view(), ec);
            if (ec) {
                QS_CORE_WARN_TAG(
                    "EditorAssetManager::Reimport",
                    "Failed to remove failed import artifact at location '{}': {}",
                    artifact, ec.message()
                );
            }
        }
    }

    bool EditorAssetManager::Reimport(const AssetID& assetId) {
        const AssetMetadata* meta = m_AssetRegistry.GetAssetMetadata(assetId);
        if (!meta) {
            QS_ERROR_TAG(
                "EditorAssetManager::Reimport",
                "Failed to reimport asset with id '{}'",
                assetId.ToString(Allocator::Frame)
            );

            return false;
        }

        const OsPath absolute = Project::GetProjectPath() / meta->FilePath;
        const std::string absoluteStr = absolute.generic_string();

        // Stamp BEFORE importing: if the file changes during the import,
        // the next refresh sees a newer mod time and reimports again.
        int64_t size = 0;
        int64_t modTime = 0;
        Optional<Hash128> hash;

        if (std::filesystem::is_directory(absolute)) {
            auto listings = ListDirectory(absolute, Allocator::Frame);
            modTime = static_cast<int64_t>(HashListing(listings));
            for (const auto& listing : listings) {
                size += listing.Size;
            }

            hash = HashDirectoryContents(absolute, listings);
        } else {
            std::error_code ec;
            size = static_cast<int64_t>(std::filesystem::file_size(absolute, ec));
            modTime = std::filesystem::last_write_time(absolute, ec).time_since_epoch().count();
            if (ec) {
                QS_ERROR_TAG(
                    "EditorAssetManager::Reimport",
                    "Failed to read source file during reimport for asset(path: '{}', id: '{}'): {}",
                    absoluteStr, assetId.ToString(Allocator::Frame), ec.message()
                );

                return false;
            }

            hash = Hash::File(absoluteStr);
        }

        if (!hash) {
            QS_ERROR_TAG(
                "EditorAssetManager::Reimport",
                "Failed to hash source file content during reimport for asset(path: '{}', id: '{}')",
                absoluteStr, assetId.ToString(Allocator::Frame)
            );

            return false;
        }

        ImportContext ctx(*meta, m_AssetRegistry, GetAllocator(Allocator::Frame));
        if (!EditorAssetImporter::Import(ctx)) {
            QS_ERROR_TAG("EditorAssetManager::Reimport", "Import failed for '{}'", meta->FilePath);
            DeleteArtifacts(ctx.GetArtifacts());

            return false;
        }

        Optional<int64_t> sidecarSize = None;
        Optional<int64_t> sidecarModTime = None;
        Optional<Hash128> sidecarHash;

        if (EditorAssetImporter::IsUsingSidecar(meta->Type)) {
            OsPath sidecarPath = SidecarHandler::GetSidecarPath(absolute);
            std::error_code ec;
            sidecarSize = static_cast<int64_t>(std::filesystem::file_size(sidecarPath, ec));
            sidecarModTime = std::filesystem::last_write_time(sidecarPath, ec).time_since_epoch().count();
            if (ec) {
                QS_ERROR_TAG(
                    "EditorAssetManager::Reimport",
                    "Failed to read '.qmeta' file during reimport for asset(path: '{}', id: '{}'): {}",
                    sidecarPath.generic_string(), assetId.ToString(Allocator::Frame), ec.message()
                );

                return false;
            }

            sidecarHash = Hash::File(sidecarPath.generic_string());
        }

        // Artifacts from the previous import that this one didn't produce
        auto previous = m_Database.GetArtifacts(assetId, GetAllocator(Allocator::Frame));

        const ImportResult result {
            .ID = assetId,
            .Path = meta->FilePath,
            .TypeName = meta->Type.GetName(),
            .SourceSize = size,
            .SourceModTime = modTime,
            .SourceHash = *hash,
            .ImporterVersion = AssetImporter::GetVersion(meta->Type),
            .MetaSize = sidecarSize,
            .MetaModTime = sidecarModTime,
            .MetaHash = sidecarHash,
            .Dependencies = ctx.GetDependencies(),
            .Artifacts = ctx.GetArtifacts(),
            .SubAssets = ctx.GetSubAssets()
        };

        if (auto r = m_Database.CommitImport(result); !r) {
            QS_ERROR_TAG(
                "EditorAssetManager::Reimport",
                "Failed to record import of '{}': {}",
                meta->FilePath, r.error()
            );

            return false;
        }

        if (previous) {
            DeleteArtifacts(ArtifactsNotIn(*previous, ctx.GetArtifacts(), Allocator::Frame));
        }

        SyncSubAssetsToRegistry(assetId, ctx.GetSubAssets());

        QS_TRACE_TAGGED("Asset (id: {}, path: {}) reimported", assetId.ToFormattedString(), meta->FilePath);

        // Hot reload loaded asset
        if (m_LoadedAssets.contains(assetId)) {
            Reload(assetId);
        }

        for (const auto& [childId, metadata] : m_AssetRegistry.GetAssetsMetadata()) {
            if (metadata.ParentId == assetId && m_LoadedAssets.contains(childId)) {
                Reload(childId);
            }
        }

        return true;
    }
}
