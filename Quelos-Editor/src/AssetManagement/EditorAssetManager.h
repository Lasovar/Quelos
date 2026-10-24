#pragma once

#include "AssetDatabase.hpp"
#include "EditorAssetImporter.h"
#include "Quelos/AssetManager/AssetManagerBase.h"
#include "Quelos/AssetManager/AssetRegistry.h"

#include "efsw/FileWatcherGeneric.hpp"
#include "Quelos/Utility/Request.h"
#include "SQLite/SQLite.hpp"

namespace QuelosEditor {
    using namespace Quelos;

    enum class SourceCheck {
        Unchanged = 0,
        StampOnly = 1,
        NeedsReimport = 2
    };

    class EditorAssetManager : public AssetManagerBase, efsw::FileWatchListener {
    public:
        EditorAssetManager();

        void Initialize();
        void UnloadAsset(AssetID assetHandle) override;

        [[nodiscard]] const AssetMetadata* GetAssetMetadata(std::string_view path) const;
        [[nodiscard]] const AssetMetadata* GetAssetMetadata(const AssetID& assetHandle) const override;

        [[nodiscard]] bool IsAssetHandleValid(const AssetID& handle) const override;
        [[nodiscard]] bool IsAssetPathValid(std::string_view path) const;

        [[nodiscard]] Vec<const AssetMetadata*> FindAssetsOfType(AssetTypeID type, AllocatorType allocatorType) const override;
        [[nodiscard]] Vec<const AssetMetadata*> FindAssetsOfType(AssetTypeID type, std::pmr::polymorphic_allocator<> allocator) const override;
        [[nodiscard]] Expected<void, String> RenameAsset(const AssetID& handle, std::string_view newPath);

        Expected<void, String> OnAssetMoved(const AssetID& handle, std::string_view newPath);
        [[nodiscard]] static bool IsAssetSupported(std::string_view path);

        [[nodiscard]] bool IsAssetLoaded(const AssetID& handle) const override;
        void RemoveAssetFromRegistry(const AssetID& assetHandle);
        void QueueReimport(AssetID assetId);
        Optional<AssetID> ResolveOwningAsset(std::string_view absolutePath) const;

        void FlushReimportQueue();
        void ProcessFileEvents();

        Vec<AssetID> ProcessAssetRegistration(std::string_view assetPath, std::pmr::polymorphic_allocator<> allocator);
        Vec<AssetID> ProcessAssetRegistration(std::string_view assetPath, AllocatorType allocatorType);

        template <typename T>
        requires (std::is_base_of_v<Asset, T>)
        void RegisterType() {
            m_AssetPools[T::GetStaticType()] = UntypedAssetPool::Create<T>(T::GetStaticType().GetName());
        }

        void* TryGet(const UntypedAssetHandle handle) override {
            const auto it = m_AssetPools.find(handle.Type);
            if (it == m_AssetPools.end()) {
                return nullptr;
            }

            const auto& pool = it->second;
            return pool.GetSlotData(pool.Data, handle);
        }

        UntypedAssetHandle Acquire(AssetID assetId) override;

        void Release(UntypedAssetHandle assetHandle) override;
        void Release(AssetID assetId) override;

        bool IsAlive(UntypedAssetHandle handle) override;

        void IncRef(UntypedAssetHandle handle) override;
        void DecRef(UntypedAssetHandle handle) override;

        void CleanupAssetMap();

        // File watch listener implementation
        void handleFileAction(efsw::WatchID watchId, const std::string& dir, const std::string& filename,
                              efsw::Action action, std::string oldFilename) override;

        void SyncSubAssetsToRegistry(const AssetID& parentId, Span<const AssetMetadata> produced);
        void RefreshAssetDatabase();
        static SourceCheck CheckSource(const SourceRecord& rec, const ScannedSource& src);

    private:

        bool Reload(const AssetID& assetId);
        bool Reimport(const AssetID& assetId);
        bool Import(const UntypedAssetPool& pool, UntypedAssetHandle assetHandle, const AssetMetadata& metadata);
        AssetID AddAssetToRegistry(std::string_view assetPath);
        void ReimportWithDependents(const HashSet<AssetID>& changed);
        bool LoadAssetRegistry();

        Expected<void, String> AddAssetToDatabase(const ImportResult& record) const;

        template <typename T, typename... Args>
        void Reconstruct(const AssetHandle<T>& handle, Args&&... args) {
            auto& pool = m_AssetPools[handle.Type];

            auto* slot = static_cast<AssetSlot<T>*>(
                pool.GetSlot(pool.Data, handle.Index)
            );

            T* obj = slot->Get();
            obj->~T();

            new(obj) T(std::forward<Args>(args)...);
        }

    private:
        AssetRegistry m_AssetRegistry;
        AssetDatabase m_Database;
        AssetMap m_LoadedAssets{Allocator::Persistent};
        AssetPools m_AssetPools{Allocator::Persistent};

        efsw::FileWatcher m_FileWatcher;
        HashMap<efsw::WatchID, AssetID> m_WatchedAssets{Allocator::Persistent};

        std::atomic<bool> m_HasFileEvents = false;
        std::mutex m_FileEventsMutex;
        Vec<std::string> m_ModifiedPaths{Allocator::Persistent};
        Vec<std::string> m_ProcessingPaths{Allocator::Persistent};
        bool m_RequestFullRefresh;

        HashSet<AssetID> m_ReimportQueue;

        efsw::WatchID m_WatchID{};
    };
}
