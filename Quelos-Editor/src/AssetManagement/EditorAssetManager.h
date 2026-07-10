#pragma once

#include "EditorAssetImporter.h"
#include "Quelos/AssetManager/AssetManagerBase.h"
#include "Quelos/AssetManager/AssetRegistry.h"

#include "efsw/FileWatcherGeneric.hpp"

namespace QuelosEditor {
    using namespace Quelos;

    class EditorAssetManager : public AssetManagerBase, efsw::FileWatchListener {
    public:
        EditorAssetManager();

        void UnloadAsset(AssetID assetHandle) override;

        [[nodiscard]] const AssetMetadata* GetAssetMetadata(std::string_view path) const;
        [[nodiscard]] const AssetMetadata* GetAssetMetadata(const AssetID& assetHandle) const override;

        [[nodiscard]] bool IsAssetHandleValid(const AssetID& handle) const override;
        [[nodiscard]] bool IsAssetPathValid(std::string_view path) const;

        [[nodiscard]] Vec<const AssetMetadata*> FindAssetsOfType(AssetTypeID type) const override;
        void RenameAsset(AssetID handle, std::string_view newPath);

        [[nodiscard]] static bool IsAssetSupported(std::string_view path);

        [[nodiscard]] bool IsAssetLoaded(const AssetID& handle) const override;
        void RemoveAssetFromRegistry(const AssetID& assetHandle);
        void Reimport(AssetID assetId);

        void FlushReimportQueue();

        Vec<const AssetMetadata*> ProcessAssetRegistration(std::string_view assetPath);

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

        void SerializeAssetRegistry();
        void DeserializeAssetRegistry();


        // File watch listener implementation
        void handleFileAction(efsw::WatchID watchId, const std::string& dir, const std::string& filename,
                              efsw::Action action, std::string oldFilename) override;
    private:

        bool Import(const UntypedAssetPool& pool, UntypedAssetHandle assetHandle, const AssetMetadata& metadata);
        const AssetMetadata* AddAssetToRegistry(std::string_view assetPath);

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
        template <typename T>
        requires (std::is_base_of_v<Asset, T>)
        static void ReimportAsset(T* asset, const AssetMetadata* assetMetadata) {
            EditorAssetImporter::TryReimportAsset(reinterpret_cast<void*>(asset), assetMetadata);
        }

    private:
        AssetRegistry m_AssetRegistry;
        AssetMap m_LoadedAssets;
        AssetPools m_AssetPools;

        efsw::FileWatcher m_FileWatcher;
        HashMap<efsw::WatchID, AssetID> m_WatchedAssets;

        HashSet<AssetID> m_ReimportQueue;

        efsw::WatchID m_WatchID;
    };
}
