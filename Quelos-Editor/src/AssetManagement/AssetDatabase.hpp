//
// Created by lasovar on 10/11/26.
//

#pragma once

#include "Quelos/AssetManager/Asset.h"
#include "Quelos/AssetManager/AssetMetadata.h"
#include "Quelos/Utility/HashMap.hpp"
#include "SQLite/SQLite.hpp"

namespace QuelosEditor {
    using namespace Quelos;

    enum class SourceState : uint8_t { Unchanged, New, Modified, Missing };

    struct SidecarStamp {
        int64_t Size;
        int64_t ModTime;
    };

    struct ScannedSource {
        String Path;
        int64_t Size = 0;
        int64_t ModTime = 0;
        Optional<SidecarStamp> Meta;
    };

    struct SourceStamp {
        AssetID ID;
        int64_t Size = 0;
        int64_t ModTime = 0;
        Optional<int64_t> MetaSize = 0;
        Optional<int64_t> MetaModTime = 0;
    };

    struct ImportResult {
        AssetID ID;
        Optional<std::string_view> Path;
        std::string_view TypeName;
        Optional<AssetID> ParentID;

        Optional<int64_t> SourceSize;
        Optional<int64_t> SourceModTime;
        Optional<Hash128> SourceHash;

        Optional<int64_t> ImporterVersion;

        Optional<int64_t> MetaSize;
        Optional<int64_t> MetaModTime;
        Optional<Hash128> MetaHash;

        Span32<const AssetID> Dependencies;
        Span32<const String> Artifacts;
        Span32<const AssetMetadata> SubAssets;
    };

    struct SourceRecord {
        AssetID ID;
        String TypeName;
        Optional<int64_t> Size; // None until first successful import
        Optional<int64_t> ModTime;
        Optional<Hash128> Hash;
        Optional<int64_t> ImporterVersion;
        Optional<int64_t> MetaSize;
        Optional<int64_t> MetaModTime;
        Optional<Hash128> MetaHash;
    };

    class AssetDatabase {
    public:
        AssetDatabase();
        [[nodiscard]] Expected<void, String> RegisterAsset(
            const AssetID& id,
            const Optional<std::string_view>& path,
            std::string_view typeName,
            const Optional<AssetID>& parent
        ) const;

        // Called on a successful import
        [[nodiscard]] Expected<void, String> CommitImport(const ImportResult& record) const;
        [[nodiscard]] Expected<void, String> MoveAsset(const AssetID& id, std::string_view oldPath, std::string_view newPath) const;

        [[nodiscard]] Expected<HashMap<String, SourceRecord>, String> LoadSourceRecords(std::pmr::memory_resource* memoryResource) const;
        [[nodiscard]] Expected<Vec<AssetMetadata>, String> LoadAssets(std::pmr::memory_resource* memoryResource) const;
        [[nodiscard]] Expected<Vec<Pair<AssetID, String>>, String> LoadAllArtifacts(std::pmr::memory_resource* memoryResource) const;
        [[nodiscard]] Expected<Vec<Pair<AssetID, String>>, String> LoadAllArtifacts(const AllocatorType allocatorType) const {
            return LoadAllArtifacts(GetAllocator(allocatorType));
        }

        [[nodiscard]] Expected<Vec<AssetMetadata>, String> LoadAssets(const AllocatorType allocatorType) const {
            return LoadAssets(GetAllocator(allocatorType));
        }

        [[nodiscard]] Expected<HashMap<String, SourceRecord>, String> LoadSourceRecords(const AllocatorType allocatorType) const {
            return LoadSourceRecords(GetAllocator(allocatorType));
        }

        Expected<void, String> GetTransitiveDependents(const AssetID& id, Vec<AssetID>& dependantsResult) const;

        Expected<Vec<GUID128>, String> GetTransitiveDependents(const AssetID& id, std::pmr::memory_resource* memoryResource) const {
            Vec<AssetID> result(memoryResource);
            return GetTransitiveDependents(id, result)
                .transform([&] { return std::move(result); });
        }

        [[nodiscard]] Expected<Vec<GUID128>, String> GetTransitiveDependents(const AssetID& id, const AllocatorType allocatorType) const {
            return GetTransitiveDependents(id, GetAllocator(allocatorType));
        }

        Expected<Vec<String>, String> GetArtifacts(const AssetID& id, std::pmr::memory_resource* memoryResource) const;


        [[nodiscard]] Expected<void, String> UpdateSourceStamp(const AssetID& id, int64_t size, int64_t modTime) const;
        [[nodiscard]] Expected<void, String> UpdateSourceStamps(Span32<const SourceStamp> stamps) const;

        [[nodiscard]] Expected<void, String> UpdateAssetPath(const AssetID& id, std::string_view newPath) const;

        [[nodiscard]] Expected<Vec<Pair<AssetID, AssetID>>, String> GetDependencyEdges(
            Span32<const AssetID> nodes, std::pmr::memory_resource* memoryResource
        ) const;

        [[nodiscard]] Expected<Vec<Pair<AssetID, AssetID>>, String> GetDependencyEdges(
            const Span32<const AssetID> nodes, const AllocatorType allocatorType
        ) const {
            return GetDependencyEdges(nodes, GetAllocator(allocatorType));
        }

    private:
        [[nodiscard]] Expected<void, String> FillTempIDTable(std::string_view table, Span<const AssetID> ids) const;

        // Must be called inside an open transaction.
        [[nodiscard]] Expected<void, String> SyncSubAssetRows(const ImportResult& record) const;

    private:
        SQLiteDatabase m_Database;
    };
} // Quelos
