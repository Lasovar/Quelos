#pragma once

#include <stduuid/uuid.h>

namespace Quelos {
    struct GUID64 {
    public:
        GUID64() = default;
        GUID64(const GUID64& other) = default;
        constexpr explicit GUID64(const uint64_t guid) : m_GUID(guid) {}
        explicit GUID64(const std::string& guidString);

        [[nodiscard]] bool IsValid() const noexcept { return m_GUID != 0; }

        operator uint64_t() const { return m_GUID; }

        [[nodiscard]] std::string ToString() const;
        [[nodiscard]] std::span<const std::byte, 8> AsBytes() const;
    public:
        static GUID64 Generate();
    private:
        uint64_t m_GUID;
        friend struct std::hash<GUID64>;
    };

    struct GUID128 {
    public:
        GUID128() = default;
        GUID128(const uuids::uuid& uuid) : m_UUID(uuid) {}
        GUID128(const std::string& uuidString);

        [[nodiscard]] bool IsValid() const noexcept { return !m_UUID.is_nil(); }

        operator bool() const { return IsValid(); }
        bool operator==(const GUID128& other) const { return m_UUID == other.m_UUID; }
        bool operator!=(const GUID128& other) const { return m_UUID != other.m_UUID; }

        [[nodiscard]] std::string ToString() const;
        [[nodiscard]] std::span<const std::byte, 16> AsBytes() const;
        [[nodiscard]] uint64_t Hash64() const;
    public:
        static GUID128 Generate();
    private:
        uuids::uuid m_UUID;
        friend struct std::hash<GUID128>;
    };
}

namespace std {
    template <>
    struct hash<Quelos::GUID64>
    {
        std::size_t operator()(const Quelos::GUID64& guid) const noexcept {
            return guid;
        }
    };

    template<>
    struct hash<Quelos::GUID128> {
        std::size_t operator()(const Quelos::GUID128& guid) const noexcept {
            return std::hash<uuids::uuid>{}(guid.m_UUID);
        }
    };
}
