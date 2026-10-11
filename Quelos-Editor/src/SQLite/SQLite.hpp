//
// Created by lasovar on 10/10/26.
//

#pragma once

#include <expected>

#include "Quelos/Core/GUID.h"
#include "Quelos/Core/SmartPointers.h"
#include "Quelos/Utility/Hash.h"
#include "Quelos/Utility/Optional.hpp"
#include "Quelos/Utility/String.hpp"

struct sqlite3;
struct sqlite3_stmt;

namespace QuelosEditor {
    struct SQLiteDatabase;
    using namespace Quelos;

    struct SQLiteStatement {
    public:
        // Binding (1-based)
        Expected<Ref<SQLiteStatement>, String> Bind(int i, int64_t v);
        Expected<Ref<SQLiteStatement>, String> Bind(int i, double v);
        Expected<Ref<SQLiteStatement>, String> Bind(int i, std::string_view v);
        Expected<Ref<SQLiteStatement>, String> Bind(int i, const GUID128& v);
        Expected<Ref<SQLiteStatement>, String> Bind(int i, const Hash128& v);
        Expected<Ref<SQLiteStatement>, String> BindNull(int i);

        template <typename... Args>
        Expected<void, String> BindAll(const Args&... args) {
            Expected<void, String> result{};
            int index = 1;
            auto bindOne = [&]<typename TArg>(const TArg& arg) {
                Expected<Ref<SQLiteStatement>, String> r = [&] {
                    if constexpr (IsOptionalV<TArg>) {
                        return arg ? Bind(index, *arg) : BindNull(index);
                    } else {
                        return Bind(index, arg);
                    }
                }();

                ++index;

                if (!r) result = Unexpected(std::move(r).error());
                return r.has_value();
            };

            (bindOne(args) && ...);
            return result;
        }

        /// Returns true while there's a row
        [[nodiscard]] Expected<bool, String> Step() const;

        void Reset() const;

        // Reading (0-based)
        [[nodiscard]] int64_t GetInt(int c) const;
        [[nodiscard]] Optional<int64_t> GetOptionalInt(int c) const;
        [[nodiscard]] double GetDouble(int c) const;
        [[nodiscard]] Optional<Hash128> GetHash(int c) const;
        [[nodiscard]] Optional<GUID128> GetGUID(int c) const;
        [[nodiscard]] Optional<String> GetText(int c, std::pmr::memory_resource* memoryResource) const;
        [[nodiscard]] Optional<String> GetText(const int c, const AllocatorType allocatorType) const {
            return GetText(c, GetAllocator(allocatorType));
        }

    private:
        explicit SQLiteStatement(sqlite3* database, sqlite3_stmt* statement);
        Expected<Ref<SQLiteStatement>, String> Check(int rc);

    private:
        struct StmtDeleter { void operator()(sqlite3_stmt* s) const noexcept; };

        sqlite3* m_Database;
        UniquePtr<sqlite3_stmt, StmtDeleter>  m_Statement;
        friend SQLiteDatabase;
    };

    struct SQLiteDatabase {
    public:
        SQLiteDatabase() = default;
        SQLiteDatabase(SQLiteDatabase&&) = default;
        SQLiteDatabase& operator=(SQLiteDatabase&&) = default;
        SQLiteDatabase(const SQLiteDatabase&) = delete;
        SQLiteDatabase& operator=(const SQLiteDatabase&) = delete;

        [[nodiscard]] Expected<SQLiteStatement, String> CreateStatement(std::string_view sql) const;
        /// Executes and returns nothing if success, otherwise a persistent String containing the error
        [[nodiscard]] Expected<void, String> Execute(std::string_view sql) const;

        template <typename... Args>
        Expected<void, String> Execute(const std::string_view sql, const Args&... args) const {
            Expected<SQLiteStatement, String> stmt = CreateStatement(sql);
            if (!stmt) {
                return Unexpected(std::move(stmt).error());
            }

            if (auto r = stmt->BindAll(args...); !r) {
                return r;
            }

            // Drain in case the statement returns rows (e.g. RETURNING); one step is enough for plain DML
            while (true) {
                Expected<bool, String> row = stmt->Step();
                if (!row) {
                    return Unexpected(std::move(row).error());
                }

                if (!*row) {
                    return {};
                }
            }
        }

    public:
        /// Opens a database, otherwise returns an error from sqlite_errmsg on a String with a persistent allocator
        static Expected<SQLiteDatabase, String> Open(std::string_view path);

    private:
        explicit SQLiteDatabase(sqlite3* database);
    private:
        struct SQLiteDatabaseDeleter { void operator()(sqlite3* db) const noexcept; };

    private:
        UniquePtr<sqlite3, SQLiteDatabaseDeleter> m_Database;
    };
}
