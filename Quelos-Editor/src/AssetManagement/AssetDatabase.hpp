//
// Created by lasovar on 10/11/26.
//

#pragma once

#include "Quelos/AssetManager/Asset.h"
#include "Quelos/Utility/HashMap.hpp"
#include "SQLite/SQLite.hpp"

namespace QuelosEditor {
    using namespace Quelos;

    enum class SourceState : uint8_t { Unchanged, New, Modified, Missing };

    struct ScannedSource {
        String Path;
        int64_t Size;
        int64_t ModTime;
    };

    struct SourceStamp {
        AssetID ID;
        int64_t Size = 0;
        int64_t ModTime = 0;
    };

    struct DatabaseAssetRecord {
        AssetID ID;
        Optional<std::string_view> Path;
        std::string_view TypeName;
        Optional<AssetID> ParentID;
        Optional<int64_t>   SourceSize;
        Optional<int64_t>   SourceModTime;
        Optional<Hash128>   SourceHash;
        Optional<int64_t>   ImporterVersion;
        Optional<Hash128>   SettingsHash;
        Span32<const AssetID> Dependencies;
        Span32<const String> Artifacts;
    };

    struct SourceRecord {
        AssetID ID;
        String TypeName;
        Optional<int64_t> Size; // None until first successful import
        Optional<int64_t> ModTime;
        Optional<Hash128> Hash;
        Optional<int64_t> ImporterVersion;
        Optional<Hash128> SettingsHash;
    };

    class AssetDatabase {
    public:
        AssetDatabase();

        [[nodiscard]] Expected<void, String> AddAssetToDatabase(const DatabaseAssetRecord& record) const;
        [[nodiscard]] Expected<HashMap<String, SourceRecord>, String> LoadSourceRecords(std::pmr::memory_resource* memoryResource) const;
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

    private:
        SQLiteDatabase m_Database;
    };
} // Quelos
