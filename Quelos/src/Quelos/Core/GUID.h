#pragma once

#define UUID_SYSTEM_GENERATOR

#include <stduuid/uuid.h>

namespace Quelos {
    struct GUID {
    public:
        GUID() = default;
        GUID(const uuids::uuid& uuid) : m_UUID(uuid) {}
        GUID(const std::string& uuidString);

        [[nodiscard]] bool IsValid() const noexcept { return !m_UUID.is_nil(); }

        operator bool() const { return IsValid(); }
        bool operator==(const GUID& other) const { return m_UUID == other.m_UUID; }
        bool operator!=(const GUID& other) const { return m_UUID != other.m_UUID; }

        [[nodiscard]] std::string ToString() const;
        [[nodiscard]] std::span<const std::byte, 16> AsByteArray() const;
    public:
        static GUID Generate();
    private:
        uuids::uuid m_UUID;
        friend struct std::hash<GUID>;
    };
}

namespace std {
    template<>
    struct hash<Quelos::GUID> {
        std::size_t operator()(const Quelos::GUID& guid) const noexcept {
            return std::hash<uuids::uuid>{}(guid.m_UUID);
        }
    };
}
