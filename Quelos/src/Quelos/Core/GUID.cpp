#include "GUID.h"

#include <charconv>

#include "xxhash.h"

namespace Quelos {
    static std::random_device s_RandomDevice;
    static std::seed_seq s_Seed{
        s_RandomDevice(), s_RandomDevice(),
        s_RandomDevice(), s_RandomDevice(),
        s_RandomDevice(), s_RandomDevice(),
        s_RandomDevice(), s_RandomDevice()
    };

    thread_local static std::mt19937 s_Engine32(s_Seed);
    thread_local static uuids::uuid_random_generator s_UUIDGen(s_Engine32);

    thread_local static std::mt19937_64 s_Engine64(s_RandomDevice());
    thread_local static std::uniform_int_distribution<uint64_t> s_UniformDistribution;

    static std::variant<uint64_t, std::errc> TryParseGuid64(const std::string_view guidString) {
        if (guidString.size() != 16) {
            return std::errc::invalid_argument;
        }

        uint64_t value = 0;
        auto [ptr, errorCode] = std::from_chars(
            guidString.data(),
            guidString.data() + guidString.size(),
            value,
            16
        );

        if (errorCode != std::errc{}) {
            return errorCode;
        }

        return value;
    }

    GUID64::GUID64(std::string_view guidString) {
        if (guidString == "null") {
            m_GUID = 0;
            return;
        }

        const std::variant<uint64_t, std::errc> guidResult = TryParseGuid64(guidString);
        if (std::holds_alternative<std::errc>(guidResult)) {
            QS_CORE_ERROR_TAG(
                "GUID",
                "failed to parse guid '{}': {}",
                guidString, std::make_error_code(std::get<std::errc>(guidResult)).message()
            );

            m_GUID = 0;
            return;
        }

        m_GUID = std::get<uint64_t>(guidResult);
    }

    std::string GUID64::ToString() const {
        return std::format("{:016X}", m_GUID);
    }

    std::string GUID64::ToFormattedString() const {
        if (!m_GUID) {
            return "null";
        }

        return ToString();
    }

    std::span<const std::byte, 8> GUID64::AsBytes() const {
        return std::span<const std::byte, 8>(reinterpret_cast<const std::byte*>(&m_GUID), 8);
    }

    GUID64 GUID64::Generate() {
        return GUID64(s_UniformDistribution(s_Engine64));
    }

    GUID128::GUID128(const std::string_view uuidString) {
        if (uuidString == "null") {
            m_UUID = {};
            return;
        }

        const std::optional<uuids::uuid> uuid = uuids::uuid::from_string(uuidString);
        QS_CORE_ASSERT(uuid.has_value(), "Invalid UUID string: '{}'", uuidString);
        m_UUID = uuid.value();
    }

    String GUID128::ToString(std::pmr::memory_resource* memoryResource) const {
        String uustr{uuids::detail::empty_guid<char>, memoryResource};

        const std::span<const std::byte, 16> dataBytes = m_UUID.as_bytes();
        const std::span<const uint8_t, 16> data(reinterpret_cast<const uint8_t*>(dataBytes.data()), dataBytes.size());

        for (uint32_t i = 0, index = 0; i < 36; ++i) {
            if (i == 8 || i == 13 || i == 18 || i == 23) {
                continue;
            }

            uustr[i] = uuids::detail::guid_encoder<char>[data[index] >> 4 & 0x0f];
            uustr[++i] = uuids::detail::guid_encoder<char>[data[index] & 0x0f];
            index++;
        }

        return uustr;
    }
    std::string GUID128::ToString() const { return uuids::to_string(m_UUID); }

    std::string GUID128::ToFormattedString() const {
        if (m_UUID.is_nil()) {
            return "null";
        }

        return ToString();
    }

    std::span<const std::byte, 16> GUID128::AsBytes() const { return m_UUID.as_bytes(); }

    uint64_t GUID128::Hash64() const {
        return XXH3_64bits(m_UUID.as_bytes().data(), 16);
    }

    GUID128 GUID128::Generate() {
        return s_UUIDGen();
    }
}
