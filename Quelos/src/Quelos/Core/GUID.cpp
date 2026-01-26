#include "GUID.h"

namespace Quelos {
    GUID::GUID(const std::string& uuidString) {
        m_UUID = uuids::uuid::from_string(uuidString).value_or(uuids::uuid());
    }

    std::string GUID::ToString() const { return to_string(m_UUID); }

    std::span<const std::byte, 16> GUID::AsByteArray() const { return m_UUID.as_bytes(); }

    GUID GUID::Generate() { return uuids::uuid_system_generator{}(); }
}
