#include "GUID.h"

namespace Quelos {

    static std::random_device s_RandomDevice;
    static std::seed_seq s_Seed{
        s_RandomDevice(), s_RandomDevice(),
        s_RandomDevice(), s_RandomDevice(),
        s_RandomDevice(), s_RandomDevice(),
        s_RandomDevice(), s_RandomDevice()
    };

    thread_local static std::mt19937 s_Engine(s_Seed);
    thread_local static uuids::uuid_random_generator s_UUIDGen(s_Engine);

    GUID::GUID(const std::string& uuidString) {
        const std::optional<uuids::uuid> uuid = uuids::uuid::from_string(uuidString);
        QS_CORE_ASSERT(uuid.has_value(), "Invalid UUID string: '{}'", uuidString);
        m_UUID = uuid.value();
    }

    std::string GUID::ToString() const { return uuids::to_string(m_UUID); }

    std::span<const std::byte, 16> GUID::AsByteArray() const { return m_UUID.as_bytes(); }

    GUID GUID::Generate() {
        return s_UUIDGen();
    }
}
