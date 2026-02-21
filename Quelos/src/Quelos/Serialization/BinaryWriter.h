#pragma once

#include <cstdint>
#include <span>
#include <type_traits>
#include <vector>

namespace Quelos::Serialization {
    class BinaryReader {
    public:
        explicit BinaryReader(const std::span<const std::byte>& data)
            : m_Data(data) {
        }

        template <typename T>
            requires std::is_trivially_copyable_v<T>
        std::optional<T> Read() {
            if (m_Offset + sizeof(T) > m_Data.size()) {
                return std::nullopt;
            }

            T value;
            std::memcpy(&value, m_Data.data() + m_Offset, sizeof(T));
            m_Offset += sizeof(T);
            return value;
        }

        std::optional<std::span<const std::byte>> ReadBytes(const size_t count) {
            if (m_Offset + count > m_Data.size()) {
                return std::nullopt;
            }

            auto slice = m_Data.subspan(m_Offset, count);
            m_Offset += count;
            return slice;
        }

    private:
        std::span<const std::byte> m_Data;
        size_t m_Offset = 0;
    };


    class BinaryWriter {
    public:
        explicit BinaryWriter(std::vector<std::byte>& buffer)
            : m_Buffer(buffer) {
        }

        template <typename T>
            requires std::is_trivially_copyable_v<T>
        void Write(const T& value) {
            static_assert(std::is_trivially_copyable_v<T>);
            const auto* ptr = reinterpret_cast<const std::byte*>(&value);
            m_Buffer.insert(m_Buffer.end(), ptr, ptr + sizeof(T));
        }

        void WriteBytes(std::span<const std::byte> bytes) const {
            m_Buffer.insert(m_Buffer.end(), bytes.begin(), bytes.end());
        }

    private:
        std::vector<std::byte>& m_Buffer;
    };

    class BinaryWriteArchive {
    public:
        static constexpr bool IsLoading = false;
        static constexpr bool IsSaving = true;

    public:
        explicit BinaryWriteArchive(BinaryWriter& writer) : m_Writer(writer) { }

        template <typename T>
        void Field(std::string_view, const T& value) {
            Value(value);
        }

        template <typename T>
        void FieldVector(const std::string&, std::vector<T>& data) const {
            m_Writer.Write(static_cast<uint32_t>(data.size()));
            m_Writer.WriteBytes(std::as_bytes(std::span(data)));
        }

        template <typename T>
        void Value(T& value) {
            m_Writer.Write(value);
        }

        // Archive API... not needed for binary
        static void BeginTuple(std::string_view) { }
        static void BeginTupleField(const std::string_view) { }
        static void EndTuple() { }

    private:
        BinaryWriter& m_Writer;
    };

    class BinaryReadArchive {
    public:
        static constexpr bool IsLoading = true;
        static constexpr bool IsSaving = false;

    public:
        explicit BinaryReadArchive(BinaryReader& reader) : m_Reader(reader) { }

        template <typename T>
        void Field(std::string_view, T& value) {
            Value(value);
        }

        template <typename T>
            requires(std::is_trivially_copyable_v<T>)
        void FieldVector(const std::string&, std::vector<T>& data) {
            const uint32_t size = m_Reader.Read<uint32_t>().value_or(0);
            const uint32_t byteSize = size * sizeof(T);
            const std::span<const std::byte> serializedBytes = m_Reader.ReadBytes(byteSize).value_or({});

            data.resize(size);
            std::memcpy(data.data(), serializedBytes.data(), byteSize);
        }

        template <typename T>
        void Value(T& value) {
            std::optional<T> result = m_Reader.Read<T>();
            if (result.has_value()) {
                value = std::move(*result);
            }
        }

        // Archive API... not needed for binary
        static void BeginTuple() { }
        static void BeginTupleField(const std::string_view) { }
        static void EndTuple() { }

    private:
        BinaryReader& m_Reader;
    };
}
