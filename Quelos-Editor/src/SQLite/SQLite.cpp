//
// Created by lasovar on 10/10/26.
//

#include "SQLite.hpp"

#include "sqlite3.h"
#include "Quelos/Core/Assert.h"
#include "Quelos/Core/Base.h"
#include "Quelos/Core/Log.h"

namespace QuelosEditor {
    SQLiteDatabase::SQLiteDatabase(sqlite3* database) : m_Database(database) { }

    Expected<SQLiteDatabase, String> SQLiteDatabase::Open(std::string_view path) {
        sqlite3* databasePtr = nullptr;
        const int returnCode = sqlite3_open_v2(
            FormatTemp("{}", path),
            &databasePtr,
            SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
            nullptr
        );

        SQLiteDatabase db(databasePtr);

        if (returnCode != SQLITE_OK) {
            return Unexpected(String(sqlite3_errmsg(db.m_Database.get()), Allocator::Persistent));
        }

        return db;
    }

    Expected<SQLiteStatement, String> SQLiteDatabase::CreateStatement(const std::string_view sql) const {
        sqlite3_stmt* raw = nullptr;
        if (sqlite3_prepare_v2(m_Database.get(), sql.data(), static_cast<int>(sql.size()), &raw, nullptr) != SQLITE_OK) {
            return Unexpected(String(sqlite3_errmsg(m_Database.get()), Allocator::Persistent));
        }

        return SQLiteStatement(m_Database.get(), raw);
    }

    Expected<void, String> SQLiteDatabase::Execute(std::string_view sql) const {
        QS_ASSERT(m_Database, "SQLiteDatabase has not been initialized!");

        char* err;
        if (sqlite3_exec(m_Database.get(), FormatTemp("{}", sql), nullptr, nullptr, &err) == SQLITE_OK) {
            return {};
        }

        String errorStr(Allocator::Persistent);
        errorStr = err ? err : "";
        sqlite3_free(err);

        return Expected<void, String>(Unexpect, std::move(errorStr));
    }


    void SQLiteDatabase::SQLiteDatabaseDeleter::operator()(sqlite3* db) const noexcept {
        sqlite3_close_v2(db);
    }

    SQLiteStatement::SQLiteStatement(sqlite3* database, sqlite3_stmt* statement)
        : m_Database(database), m_Statement(statement) {}

    Expected<Ref<SQLiteStatement>, String> SQLiteStatement::Check(const int rc) {
        if (rc == SQLITE_OK) {
            return std::ref(*this);
        }

        return Unexpected(String(sqlite3_errmsg(m_Database), Allocator::Persistent));
    }

    void SQLiteStatement::StmtDeleter::operator()(sqlite3_stmt* s) const noexcept { sqlite3_finalize(s); }

    Expected<Ref<SQLiteStatement>, String> SQLiteStatement::Bind(const int i, const int64_t v) {
        return Check(sqlite3_bind_int64(m_Statement.get(), i, v));
    }

    Expected<Ref<SQLiteStatement>, String> SQLiteStatement::Bind(const int i, const double v) {
        return Check(sqlite3_bind_double(m_Statement.get(), i, v));
    }

    Expected<Ref<SQLiteStatement>, String> SQLiteStatement::Bind(const int i, const std::string_view v) {
        return Check(sqlite3_bind_text(m_Statement.get(), i, v.data(), static_cast<int>(v.size()), SQLITE_TRANSIENT));
    }

    Expected<Ref<SQLiteStatement>, String> SQLiteStatement::BindNull(const int i) {
        return Check(sqlite3_bind_null(m_Statement.get(), i));
    }

    Expected<bool, String> SQLiteStatement::Step() const {
        const int rc = sqlite3_step(m_Statement.get());
        if (rc == SQLITE_ROW) {
            return true;
        }

        if (rc == SQLITE_DONE) {
            return false;
        }

        return Unexpected(String(sqlite3_errmsg(m_Database), Allocator::Persistent));
    }

    void SQLiteStatement::Reset() const {
        sqlite3_reset(m_Statement.get());
        sqlite3_clear_bindings(m_Statement.get());
    }

    int64_t SQLiteStatement::GetInt(const int c) const {
        return sqlite3_column_int64(m_Statement.get(), c);
    }

    Optional<int64_t> SQLiteStatement::GetOptionalInt(const int c) const {
        if (sqlite3_column_type(m_Statement.get(), c) == SQLITE_NULL) {
            return {};
        }

        return sqlite3_column_int64(m_Statement.get(), c);
    }

    double SQLiteStatement::GetDouble(const int c) const {
        return sqlite3_column_double(m_Statement.get(), c);
    }

    Optional<Hash128> SQLiteStatement::GetHash(const int c) const {
        if (sqlite3_column_type(m_Statement.get(), c) == SQLITE_NULL) {
            return None;
        }

        if (sqlite3_column_bytes(m_Statement.get(), c) != 16) {
            return None;
        }

        Hash128 g;
        std::memcpy(&g, sqlite3_column_blob(m_Statement.get(), c), 16);
        return g;
    }

    Expected<Ref<SQLiteStatement>, String> SQLiteStatement::Bind(const int i, const GUID128& v) {
        static_assert(sizeof(GUID128) == 16 && std::is_trivially_copyable_v<GUID128>);
        return Check(sqlite3_bind_blob(m_Statement.get(), i, &v, 16, SQLITE_TRANSIENT));
    }

    Expected<Ref<SQLiteStatement>, String> SQLiteStatement::Bind(const int i, const Hash128& v) {
        static_assert(sizeof(Hash128) == 16 && std::is_trivially_copyable_v<Hash128>);
        return Check(sqlite3_bind_blob(m_Statement.get(), i, &v, 16, SQLITE_TRANSIENT));
    }

    Optional<GUID128> SQLiteStatement::GetGUID(const int c) const {
        if (sqlite3_column_type(m_Statement.get(), c) == SQLITE_NULL) {
            return {};
        }

        if (sqlite3_column_bytes(m_Statement.get(), c) != 16) {
            return {};
        }

        GUID128 g;
        std::memcpy(&g, sqlite3_column_blob(m_Statement.get(), c), 16);
        return g;
    }

    Optional<String> SQLiteStatement::GetText(const int c, std::pmr::memory_resource* memoryResource) const {
        const char* p = reinterpret_cast<const char*>(sqlite3_column_text(m_Statement.get(), c));
        if (!p) {
            return None;
        }

        return String(p, sqlite3_column_bytes(m_Statement.get(), c), memoryResource);
    }
}
