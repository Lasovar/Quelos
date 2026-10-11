//
// Created by lasovar on 10/11/26.
//

#include "qspch.h"
#include "AssetDatabase.hpp"

#include "Quelos/Project/Project.h"

namespace QuelosEditor {
    AssetDatabase::AssetDatabase() {
        auto databaseExpect = SQLiteDatabase::Open(
            FormatTemp("{}/AssetDatabase.sqlite", Project::GetLibraryPath().generic_string())
        );

        QS_ASSERT(databaseExpect, "EditorAssetManager Failed to open asset database: {}", databaseExpect.error());

        m_Database = std::move(*databaseExpect);
        constexpr std::string_view databaseInitialization = R"(
PRAGMA foreign_keys = ON;
PRAGMA journal_mode = WAL;

CREATE TABLE IF NOT EXISTS Assets (
    AssetID       BLOB    NOT NULL PRIMARY KEY CHECK (length(AssetID) = 16),
    AssetPath     TEXT    UNIQUE,              -- NULL for sub-assets
    AssetTypeName TEXT    NOT NULL,
    ParentAssetID BLOB    REFERENCES Assets(AssetID) ON DELETE CASCADE
                          DEFERRABLE INITIALLY DEFERRED
                          CHECK (ParentAssetID IS NULL OR length(ParentAssetID) = 16),
    SourceSize    INTEGER,
    SourceModTime INTEGER,
    SourceHash    BLOB    CHECK (SourceHash   IS NULL OR length(SourceHash)   = 16),
    ImporterVersion INTEGER,
    SettingsHash  BLOB    CHECK (SettingsHash IS NULL OR length(SettingsHash) = 16),
) STRICT, WITHOUT ROWID;

CREATE INDEX IF NOT EXISTS IdxAssetsParent ON Assets(ParentAssetID);
CREATE INDEX IF NOT EXISTS IdxAssetsType   ON Assets(AssetTypeName);

CREATE TABLE IF NOT EXISTS AssetDependencies (
    AssetID     BLOB NOT NULL REFERENCES Assets(AssetID) ON DELETE CASCADE,
    DependsOnID BLOB NOT NULL,
    PRIMARY KEY (AssetID, DependsOnID)
) STRICT, WITHOUT ROWID;

CREATE INDEX IF NOT EXISTS IdxDepsReverse ON AssetDependencies(DependsOnID);

CREATE TABLE IF NOT EXISTS AssetArtifacts (
    AssetID      BLOB NOT NULL REFERENCES Assets(AssetID) ON DELETE CASCADE,
    ArtifactPath TEXT NOT NULL UNIQUE,
    PRIMARY KEY (AssetID, ArtifactPath)
) STRICT, WITHOUT ROWID;
)";

        auto result = m_Database.Execute(databaseInitialization);
        QS_ASSERT(result, "Failed to initialize database: {}", result.error());

    }

    Expected<void, String> AssetDatabase::AddAssetToDatabase(const DatabaseAssetRecord& record) const {
        if (auto r = m_Database.Execute("BEGIN IMMEDIATE;"); !r) {
            return r;
        }

        auto result = [&]() -> Expected<void, String> {
            // Asset row
            auto upsert = m_Database.CreateStatement(R"(
    INSERT INTO Assets (AssetID, AssetPath, AssetTypeName, ParentAssetID,
                        SourceSize, SourceModTime, SourceHash, ImporterVersion, SettingsHash)
    VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9)
    ON CONFLICT (AssetID) DO UPDATE SET
        AssetPath       = excluded.AssetPath,
        AssetTypeName   = excluded.AssetTypeName,
        ParentAssetID   = excluded.ParentAssetID,
        SourceSize      = excluded.SourceSize,
        SourceModTime   = excluded.SourceModTime,
        SourceHash      = excluded.SourceHash,
        ImporterVersion = excluded.ImporterVersion,
        SettingsHash    = excluded.SettingsHash;
)");

            if (!upsert) {
                return Unexpected(std::move(upsert).error());
            }

            if (Expected<void, String> r = upsert->BindAll(
                    record.ID,
                    record.Path,
                    record.TypeName,
                    record.ParentID,
                    record.SourceSize,
                    record.SourceModTime,
                    record.SourceHash,
                    record.ImporterVersion,
                    record.SettingsHash
                );
                !r)
            {
                return r;
            }

            if (auto r = upsert->Step(); !r) {
                return Unexpected(std::move(r).error());
            }

            // Dependencies: replace the whole set
            if (Expected<void, String> r = m_Database.Execute("DELETE FROM AssetDependencies WHERE AssetID = ?;", record.ID); !r) {
                return r;
            }

            Expected<SQLiteStatement, String> insertDep = m_Database.CreateStatement(
                "INSERT INTO AssetDependencies (AssetID, DependsOnID) VALUES (?, ?);"
            );

            if (!insertDep) {
                return Unexpected(std::move(insertDep).error());
            }

            for (const GUID128& dep : record.Dependencies) {
                if (auto r = insertDep->BindAll(record.ID, dep); !r) {
                    return r;
                }

                if (auto r = insertDep->Step(); !r) {
                    return Unexpected(std::move(r).error());
                }

                insertDep->Reset();
            }

            // Artifacts: replace the whole set
            if (Expected<void, String> r = m_Database.Execute( "DELETE FROM AssetArtifacts WHERE AssetID = ?;", record.ID); !r) {
                return r;
            }

            Expected<SQLiteStatement, String> insertArtifact = m_Database.CreateStatement(
                "INSERT INTO AssetArtifacts (AssetID, ArtifactPath) VALUES (?, ?);"
            );

            if (!insertArtifact) {
                return Unexpected(std::move(insertArtifact).error());
            }

            for (const String& path : record.Artifacts) {
                if (Expected<void, String> r = insertArtifact->BindAll(record.ID, std::string_view(path)); !r) {
                    return r;
                }

                if (auto r = insertArtifact->Step(); !r) {
                    return Unexpected(std::move(r).error());
                }

                insertArtifact->Reset();
            }

            return {};
        }();

        if (!result) {
            [[maybe_unused]] const auto rollbackResult = m_Database.Execute("ROLLBACK;");
            QS_ASSERT(rollbackResult, "Asset database rollback failed: {}", rollbackResult.error());
            return result;
        }

        return m_Database.Execute("COMMIT;");
    }

    Expected<HashMap<String, SourceRecord>, String> AssetDatabase::LoadSourceRecords(
        std::pmr::memory_resource* memoryResource
    ) const {
        auto query = m_Database.CreateStatement(R"(
        SELECT AssetPath, AssetID, AssetTypeName,
               SourceSize, SourceModTime, SourceHash, ImporterVersion, SettingsHash
        FROM Assets
        WHERE AssetPath IS NOT NULL;)"
        );

        if (!query) {
            return Unexpected(std::move(query).error());
        }

        HashMap<String, SourceRecord> records(memoryResource);
        while (true) {
            auto row = query->Step();
            if (!row) {
                return Unexpected(std::move(row).error());
            }

            if (!*row) {
                break;
            }

            Optional<String> path = query->GetText(0, memoryResource);
            Optional<AssetID> id = query->GetGUID(1);
            if (!path || !id) {
                continue;
            }

            records.emplace(
                std::move(*path),
                SourceRecord {
                    .ID = *id,
                    .TypeName = query->GetText(2, memoryResource).value_or(String{}),
                    .Size = query->GetOptionalInt(3),
                    .ModTime = query->GetOptionalInt(4),
                    .Hash = query->GetHash(5),
                    .ImporterVersion = query->GetOptionalInt(6),
                    .SettingsHash = query->GetHash(7),
                }
            );
        }

        return records;
    }

    Expected<void, String> AssetDatabase::GetTransitiveDependents(const AssetID& id, Vec<AssetID>& dependantsResult) const {
        Expected<SQLiteStatement, String> query = m_Database.CreateStatement(R"(
        WITH RECURSIVE Dependents(ID) AS (
            SELECT AssetID FROM AssetDependencies WHERE DependsOnID = ?1
            UNION
            SELECT d.AssetID FROM AssetDependencies d
            JOIN Dependents ON d.DependsOnID = Dependents.ID
        )
        SELECT ID FROM Dependents;
        )");

        if (!query) {
            return Unexpected(std::move(query).error());
        }

        if (auto r = query->BindAll(id); !r) {
            return Unexpected(std::move(r).error());
        }

        while (true) {
            Expected<bool, String> row = query->Step();

            if (!row) {
                return Unexpected(std::move(row).error());
            }

            if (!*row) {
                break;
            }

            if (auto dep = query->GetGUID(0)) {
                dependantsResult.push_back(*dep);
            }
        }

        return {};
    }

    Expected<void, String> AssetDatabase::FillTempIDTable(std::string_view table, const Span<const AssetID> ids) const {
        const String create(
            FormatTemp(
                "CREATE TEMP TABLE IF NOT EXISTS {} (ID BLOB PRIMARY KEY) WITHOUT ROWID; DELETE FROM {};",
                table, table
            ),
            Allocator::Frame
        );

        if (auto r = m_Database.Execute(create); !r) {
            return r;
        }

        Expected<SQLiteStatement, String> insert = m_Database.CreateStatement(
            FormatTemp("INSERT OR IGNORE INTO {} (ID) VALUES (?);", table)
        );

        if (!insert) {
            return Unexpected(std::move(insert).error());
        }

        for (const GUID128& id : ids) {
            if (auto r = insert->BindAll(id); !r) {
                return r;
            }

            if (auto r = insert->Step(); !r) {
                return Unexpected(std::move(r).error());
            }

            insert->Reset();
        }

        return {};
    }

    Expected<Vec<Pair<AssetID, AssetID>>, String> AssetDatabase::GetDependencyEdges(
        const Span32<const AssetID> nodes,
        std::pmr::memory_resource* memoryResource
    ) const {
        if (auto r = FillTempIDTable("ReimportSet", nodes); !r) {
            return Unexpected(std::move(r).error());
        }

        auto query = m_Database.CreateStatement(R"(
        SELECT DISTINCT d.AssetID, COALESCE(dep.ParentAssetID, d.DependsOnID)
        FROM AssetDependencies d
        LEFT JOIN Assets dep ON dep.AssetID = d.DependsOnID
        JOIN ReimportSet a ON a.ID = d.AssetID
        JOIN ReimportSet b ON b.ID = COALESCE(dep.ParentAssetID, d.DependsOnID);
    )");

        if (!query) {
            return Unexpected(std::move(query).error());
        }

        Vec<Pair<AssetID, AssetID>> edges(memoryResource);
        while (true) {
            Expected<bool, String> row = query->Step();
            if (!row) {
                return Unexpected(std::move(row).error());
            }

            if (!*row) {
                break;
            }

            Optional<AssetID> asset = query->GetGUID(0);
            Optional<AssetID> dependsOn = query->GetGUID(1);

            if (asset && dependsOn) {
                edges.emplace_back(*asset, *dependsOn);
            }
        }

        return edges;
    }

    Expected<void, String> AssetDatabase::UpdateSourceStamp(const AssetID& id, const int64_t size, const int64_t modTime) const {
        return m_Database.Execute(
            "UPDATE Assets SET SourceSize = ?, SourceModTime = ? WHERE AssetID = ?;",
            size, modTime, id
        );
    }

    Expected<void, String> AssetDatabase::UpdateSourceStamps(const Span32<const SourceStamp> stamps) const {
        if (Expected<void, String> r = m_Database.Execute("BEGIN IMMEDIATE;"); !r) {
            return r;
        }

        auto result = [&]() -> Expected<void, String> {
            Expected<SQLiteStatement, String> update = m_Database.CreateStatement(
                "UPDATE Assets SET SourceSize = ?, SourceModTime = ? WHERE AssetID = ?;"
            );

            if (!update) {
                return Unexpected(std::move(update).error());
            }

            for (const SourceStamp& s : stamps) {
                if (auto r = update->BindAll(s.Size, s.ModTime, s.ID); !r) {
                    return r;
                }

                if (auto r = update->Step(); !r) {
                    return Unexpected(std::move(r).error());
                }

                update->Reset();
            }

            return {};
        }();

        if (!result) {
            [[maybe_unused]] auto rollback = m_Database.Execute("ROLLBACK;");
            QS_ASSERT(rollback, "Asset database rollback failed: {}", rollback.error());
            return result;
        }

        return m_Database.Execute("COMMIT;");
    }

    Expected<void, String> AssetDatabase::UpdateAssetPath(const AssetID& id, const std::string_view newPath) const {
        return m_Database.Execute(
            "UPDATE Assets SET AssetPath = ? WHERE AssetID = ?;",
            newPath, id
        );
    }
} // Quelos
