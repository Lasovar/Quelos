#include "qspch.h"
#include "EditorAssetManager.h"

#include "AssetImporters/ModelImporter.h"
#include "AssetImporters/SceneImporter.h"
#include "EditorAssetImporter.h"
#include "ImportContext.hpp"
#include "xxhash.h"
#include "AssetImporters/MaterialImporter.h"
#include "AssetImporters/ShaderImporter.h"
#include "Quelos/AssetManager/AssetRegistryExtensions.h"
#include "Quelos/AssetManager/TextureImporter.h"
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

    void EditorAssetManager::FlushReimportQueue() {
        ReimportWithDependents(m_ReimportQueue);
        m_ReimportQueue.clear();
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

        if (!EditorAssetImporter::LoadAsset(LoadContext(metadata, slotData))) {
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
            QS_CORE_ASSERT(false, "[EditorAssetManager::Acquire] Invalid asset handle {}", assetId.ToString());
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

    void EditorAssetManager::RenameAsset(const AssetID handle, const std::string_view newPath) {
        auto& metadata = m_AssetRegistry.GetAssetsMetadata();
        const auto it = metadata.find(handle);
        if (it == metadata.end()) {
            return;
        }

        const OsPath oldOsPath = Project::GetProjectPath() / it->second.FilePath;
        const OsPath newOsPath = Project::GetProjectPath() / newPath;

        std::filesystem::rename(oldOsPath, newOsPath);
        it->second.FilePath = newPath;
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

        AssetImporter::RegisterAssetImporter(TextureImporter::GetImporterConfig());
        RegisterType<Texture2D>();
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

    void EditorAssetManager::SerializeAssetRegistry() {
        using namespace Serialization;

        OsPath assetRegistryPath = Project::GetProjectSettingsPath() / k_AssetRegistryFilename;

        std::string buffer;
        StringQuelWriter writer(buffer);

        auto& assetRegistry = m_AssetRegistry.GetAssetsMetadata();
        Vec<AssetMetadata> assets(Allocator::Frame);
        assets.reserve(assetRegistry.size());

        std::vector<AssetMetadata> s;
        assets.append_range(assetRegistry | std::views::values);
        std::ranges::sort(assets,
                          [](const AssetMetadata& a, const AssetMetadata& b) {
                              return a.Type < b.Type;
                          });

        for (auto& metadata : assets) {
            writer.Write(SectionEvent{metadata.Type.GetName()});
            writer.CloseSection();
            writer.WriteField("handle", UnquotedString{metadata.Handle.ToString()});
            if (metadata.IsSubAsset()) {
                writer.WriteField("parent", UnquotedString{metadata.ParentId.ToFormattedString()});
            }
            writer.WriteField("path", metadata.FilePath);
        }

        if (std::ofstream file(assetRegistryPath, std::ios::binary); file) {
            file.write(buffer.c_str(), buffer.size());
        }
        else {
            QS_CORE_ERROR_TAG(
                "AssetManager::SerializeAssetRegistry",
                "Failed to open registry file '{}'",
                assetRegistryPath.string()
            );
        }
    }

    void EditorAssetManager::DeserializeAssetRegistry() {
        using namespace Serialization;

        OsPath assetRegistryPath = Project::GetProjectSettingsPath() / k_AssetRegistryFilename;
        std::ifstream file(assetRegistryPath, std::ios::binary | std::ios::ate);
        if (!file) {
            QS_CORE_ERROR_TAG(
                "AssetManager::DeserializeAssetRegistry",
                "Failed to open registry file '{}'",
                assetRegistryPath.string()
            );

            return;
        }

        const size_t fileSize = file.tellg();
        file.seekg(0);

        std::string assetRegistryBuffer;
        assetRegistryBuffer.resize(fileSize);
        file.read(assetRegistryBuffer.data(), fileSize);

        QuelReader reader(assetRegistryBuffer);

        AssetMetadata metadata;
        std::string_view currentField;

        HashMap<AssetID, AssetMetadata>& assetsMetadata = m_AssetRegistry.GetAssetsMetadata();
        for (auto&& parserEvent : reader.Parse()) {
            std::visit([&]<typename TEvent>(const TEvent& e) {
                           using T = std::decay_t<TEvent>;

                           if constexpr (std::is_same_v<T, SectionEvent>) {
                               if (metadata.Handle) {
                                   assetsMetadata[metadata.Handle] = metadata;
                                   metadata = {};
                               }

                               if (const AssetType assetTypeResult = GetAssetType(std::string(e.Name))) {
                                   metadata.Type = assetTypeResult;
                               }
                           }
                           else if constexpr (std::is_same_v<T, FieldEvent>) {
                               currentField = e.Path;
                           }
                           else if constexpr (std::is_same_v<T, ValueEvent>) {
                               if (const std::string_view* valueResult = std::get_if<std::string_view>(&e.Value)) {
                                   if (currentField == "handle") {
                                       metadata.Handle = AssetID(*valueResult);
                                   }
                                   else if (currentField == "path") {
                                       metadata.FilePath = *valueResult;
                                   }
                                   else if (currentField == "parent") {
                                       metadata.ParentId = AssetID(*valueResult);
                                   }
                               }
                           }
                       },
                       parserEvent);
        }

        if (metadata.Handle) {
            assetsMetadata[metadata.Handle] = metadata;
        }

        m_WatchID = m_FileWatcher.addWatch(
            (Project::GetAssetsPath()).generic_string(),
            this,
            true
        );

        m_FileWatcher.watch();
    }

    void EditorAssetManager::handleFileAction(
        const efsw::WatchID watchId, const std::string& dir,
        const std::string& filename, const efsw::Action action,
        std::string oldFilename
    ) {
        switch (action) {
        case efsw::Action::Add:
        case efsw::Action::Delete:
            break;
        case efsw::Action::Modified: {
            const AssetMetadata* metadata = GetAssetMetadata(
                std::filesystem::relative(
                    FormatTemp("{}/{}", dir, filename),
                    Project::GetProjectPath()
                ).generic_string()
            );

            if (!metadata || metadata->ParentId) {
                break;
            }

            m_ReimportQueue.emplace(metadata->Handle);
            break;
        }
        default:
            break;
        }
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

    void EditorAssetManager::ReimportWithDependents(const HashSet<AssetID>& changed) {
        static constexpr auto Tag = "EditorAssetManager::ReimportWithDependents";

        if (changed.empty()) {
            return;
        }

        static auto GetRootAsset = [&](const AssetID id) -> AssetID {
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

                if (assetId == target) {
                    continue;
                }

                if (!withDependents.contains(target)) {
                    continue; // not being reimported
                }

                if (edgeSet.insert({id, target}).second) {
                    edges->push_back({id, target});
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

        for (const auto idIndex : sorted.Order) {
            Reimport(graph.Nodes[idIndex]);
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
        static constexpr auto Tag = "EditorAssetManager::RefreshAssetDatabase";
        QS_PROFILE_SCOPED();

        // path -> {ID, Size, ModTime, Hash, ImporterVersion, ...}
        auto knownResult = m_Database.LoadSourceRecords(Allocator::Frame);
        if (!knownResult) {
            QS_ERROR_TAG(Tag, "Unable to load existing database records: {}", knownResult.error());
            QS_ASSERT(false);
            return;
        }

        auto& known = *knownResult;
        HashMap<AssetID, const SourceRecord*> knownByID(Allocator::Frame);
        knownByID.reserve(known.size());
        for (const auto& rec : known | std::views::values) {
            knownByID.emplace(rec.ID, &rec);
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

            if (!IsAssetSupported(absoluteEntryPath.generic_string())) {
                continue;
            }

            String relativeAssetPath(std::filesystem::relative(absoluteEntryPath, Project::GetProjectPath())
                                     .lexically_normal()
                                     .generic_string(),
                                     Allocator::Frame);

            ScannedSource src{
                .Path = String(relativeAssetPath, Allocator::Frame),
                .Size = static_cast<int64_t>(it->file_size(ec)),
                .ModTime = it->last_write_time(ec).time_since_epoch().count()
            };

            if (ec) {
                // vanished or locked mid-scan, next refresh catches it
                ec.clear();
                continue;
            }

            if (auto found = known.find(relativeAssetPath); found != known.end()) {
                auto& record = found->second;
                seenIDs.insert(record.ID);

                switch (CheckSource(record, src)) {
                case SourceCheck::Unchanged:
                    break;
                case SourceCheck::StampOnly:
                    stampUpdates.push_back({.ID = record.ID, .Size = src.Size, .ModTime = src.ModTime});
                    break;
                case SourceCheck::NeedsReimport:
                    toImport.insert(record.ID);
                    break;
                }

                continue;
            }

            if (AssetID existing = EditorAssetImporter::ReadAssetHandle(relativeAssetPath);
                existing && knownByID.contains(existing)
            ) {
                seenIDs.insert(existing);
                if (auto r = m_Database.UpdateAssetPath(existing, relativeAssetPath); !r) {
                    QS_ERROR_TAG(
                        Tag,
                        "Failed to record move of {}: {}",
                        relativeAssetPath,
                        r.error()
                    );
                }

                RenameAsset(existing, relativeAssetPath);

                const SourceRecord& record = *knownByID.at(existing);
                SourceCheck movedCheck = CheckSource(record, src);
                if (movedCheck == SourceCheck::NeedsReimport) {
                    toImport.insert(existing);
                }
                else if (movedCheck == SourceCheck::StampOnly) {
                    stampUpdates.emplace_back(existing, src.Size, src.ModTime);
                }

                continue;
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

        ReimportWithDependents(toImport);
    }

    SourceCheck EditorAssetManager::CheckSource(const SourceRecord& rec, const ScannedSource& src) {
        if (rec.ImporterVersion != AssetImporter::GetVersion(Hash::Fnv1a32(rec.TypeName))) {
            return SourceCheck::NeedsReimport;
        }

        /*
        TODO:
        if (rec.SettingsHash != LoadImportSettingsHash(src.Path)) {
            return SourceCheck::NeedsReimport;
        }
        */

        /*
        TODO:
        if (!ArtifactsExist(rec.ID)) {
            return SourceCheck::NeedsReimport;   // library folder deleted
        }
        */

        if (rec.Size == src.Size && rec.ModTime == src.ModTime) {
            return SourceCheck::Unchanged; // fast path
        }

        // Metadata changed: confirm with the content hash
        const OsPath& absolutePath =  Project::GetProjectPath() / src.Path.view();
        Optional<Hash128> hash = None;

        if (std::filesystem::is_directory(absolutePath)) {
            auto listing = ListDirectory(absolutePath, Allocator::Frame);
            hash = HashDirectoryContents(absolutePath, listing);
        } else {
            hash = Hash::File(absolutePath.generic_string());
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
                return Import(pool, assetHandle, *metadata);
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
            .SettingsHash = None/*TODO: LoadImportSettingsHash(meta->FilePath)*/,
            .Dependencies = ctx.GetDependencies(),
            .Artifacts = ctx.GetArtifacts(),
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
