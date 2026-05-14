#pragma once

#include <cstdint>
#include <span>
#include <type_traits>
#include <vector>

#include "Quelos/AssetManager/Asset.h"
#include "Quelos/AssetManager/AssetManager.h"
#include "Quelos/AssetManager/AssetRef.h"
#include "Quelos/AssetManager/SoftRef.h"
#include "Quelos/Utility/Hash.h"

namespace Quelos::Serialization {
    class QS_API BinaryReader {
    public:
        explicit BinaryReader(const BufferView& data)
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

        [[nodiscard]] BufferView ReadBytes(const size_t count) {
            if (count > m_Data.size() - m_Offset) {
                return {};
            }

            const BufferView slice = m_Data.subspan(m_Offset, count);
            m_Offset += count;
            return slice;
        }

        /// @return The remaining bytes in the buffer from the current m_Offset
        [[nodiscard]] BufferView ReadBytesAll() {
            if (m_Offset >= m_Data.size()) {
                return {};
            }

            const BufferView slice = m_Data.subspan(m_Offset);
            m_Offset = m_Data.size();
            return slice;
        }

        [[nodiscard]] bool HasRemaining() const {
            return m_Offset < m_Data.size();
        }

    private:
        BufferView m_Data;
        size_t m_Offset = 0;
    };

    class QS_API BinaryWriter {
    public:
        explicit BinaryWriter(Vec<byte>& buffer)
            : m_Buffer(buffer) {
        }

        template <typename T>
            requires std::is_trivially_copyable_v<T>
        void Write(const T& value) {
            static_assert(std::is_trivially_copyable_v<T>);
            const auto* ptr = reinterpret_cast<const std::byte*>(&value);
            m_Buffer.insert(m_Buffer.end(), ptr, ptr + sizeof(T));
        }

        void WriteBytes(BufferView bytes) const {
            m_Buffer.insert(m_Buffer.end(), bytes.begin(), bytes.end());
        }

    private:
        Vec<byte>& m_Buffer;
    };

    class QS_API BinaryWriteArchive {
    public:
        static constexpr bool IsLoading = false;
        static constexpr bool IsSaving = true;

    public:
        explicit BinaryWriteArchive(BinaryWriter& writer) : m_Writer(writer) {
        }

        template <typename T>
        void Field(const std::string_view name, T& value) {
            m_Writer.Write(Hash::Fnv1a64(name));
            WriteValue(value);
        }

        template <typename T>
        void FieldVector(const std::string_view name, std::vector<T>& data) const {
            auto bytes = std::as_bytes(std::span(data));
            constexpr uint64_t itemSize = sizeof(T);

            m_Writer.Write(Hash::Fnv1a64(name));
            m_Writer.Write(static_cast<uint64_t>(sizeof(itemSize) + bytes.size()));

            m_Writer.Write(itemSize);
            m_Writer.WriteBytes(bytes);
        }

        // Archive API... not needed for binary
        template <typename T>
        static void Value(T&) { }
        static void BeginTuple(std::string_view) { }
        static void BeginTupleField(const std::string_view) { }
        static void EndTuple() { }

    private:
        template <typename T>
        void WriteValue(T& value) {
            if constexpr (std::is_enum_v<T>) {
                using Underlying = std::underlying_type_t<T>;
                m_Writer.Write(static_cast<uint64_t>(sizeof(Underlying)));
                m_Writer.Write(static_cast<Underlying>(value));
            }
            else {
                m_Writer.Write(static_cast<uint64_t>(sizeof(T)));
                m_Writer.Write(value);
            }
        }

        void WriteValue(float2& value) {
            m_Writer.Write(sizeof(pfloat2));
            m_Writer.Write(pfloat2(value));
        }

        void WriteValue(float3& value) {
            m_Writer.Write(sizeof(pfloat3));
            m_Writer.Write(pfloat3(value));
        }

        void WriteValue(float4& value) {
            m_Writer.Write(sizeof(pfloat4));
            m_Writer.Write(pfloat4(value));
        }

        void WriteValue(quaternion& value) {
            m_Writer.Write(sizeof(pfloat4));
            m_Writer.Write(pfloat4(value.xyzw));
        }

        template <typename T>
        void WriteValue(AssetRef<T>& value) {
            m_Writer.Write(static_cast<uint64_t>(sizeof(AssetID)));
            m_Writer.Write(value.GetAssetID());
        }

        template <typename T>
        void WriteValue(SoftRef<T>& value) {
            if constexpr (std::is_base_of_v<Asset, T>) {
                m_Writer.Write(static_cast<uint64_t>(sizeof(AssetID)));
                m_Writer.Write(value.GetAssetID());
            }
            else {
                static_assert(!sizeof(T), "SoftRef<T> only supported for Asset types");
            }
        }

        BinaryWriter& m_Writer;
    };

    class QS_API BinaryReadArchive {
    public:
        static constexpr bool IsLoading = true;
        static constexpr bool IsSaving = false;

    public:
        explicit BinaryReadArchive(HashMap<uint64_t, BufferView>& fieldsMap)
            : m_FieldsMap(fieldsMap) { }

        template <typename T>
        void Field(const std::string_view name, T& value) {
            auto it = m_FieldsMap.find(Hash::Fnv1a64(name));
            if (it == m_FieldsMap.end()) {
                return;
            }

            BinaryReader reader(it->second);
            ReadValue(reader, value);
        }

        template <typename T>
            requires(std::is_trivially_copyable_v<T>)
        void FieldVector(const std::string_view name, std::vector<T>& data) {
            auto it = m_FieldsMap.find(Hash::Fnv1a64(name));
            if (it == m_FieldsMap.end()) {
                return;
            }

            BinaryReader reader(it->second);

            const uint32_t size = reader.Read<uint64_t>().value_or(0);
            if (size != sizeof(T)) {
                return;
            }

            const BufferView serializedBytes = reader.ReadBytesAll();
            data.resize(serializedBytes.size() / sizeof(T));
            std::memcpy(data.data(), serializedBytes.data(), serializedBytes.size());
        }

        // Archive API... not needed for binary
        template <typename T>
        static void Value(T&) { }
        static void BeginTuple() { }
        static void BeginTupleField(const std::string_view) { }
        static void EndTuple() { }
    private:
        template <typename T>
        void ReadValue(BinaryReader& reader, T& value) {
            if constexpr (std::is_enum_v<T>) {
                using Underlying = std::underlying_type_t<T>;

                auto raw = reader.Read<Underlying>();
                if (!raw.has_value()) {
                    return;
                }

                value = static_cast<T>(*raw);
            }
            else {
                std::optional<T> result = reader.Read<T>();
                if (result.has_value()) {
                    value = std::move(*result);
                }
            }
        }

        static void ReadValue(BinaryReader& reader, float2& value) {
            if (const std::optional<pfloat2> result = reader.Read<pfloat2>(); result) {
                const pfloat2 resultValue = result.value();
                value = float2(resultValue.x, resultValue.y);
            }
        }

        static void ReadValue(BinaryReader& reader, float3& value) {
            if (const std::optional<pfloat3> result = reader.Read<pfloat3>(); result) {
                const pfloat3 resultValue = result.value();
                value = float3(resultValue.x, resultValue.y, resultValue.z);
            }
        }

        static void ReadValue(BinaryReader& reader, float4& value) {
            if (const std::optional<pfloat4> result = reader.Read<pfloat4>(); result) {
                const pfloat4 resultValue = result.value();
                value = float4(resultValue.x, resultValue.y, resultValue.z, resultValue.w);
            }
        }

        static void ReadValue(BinaryReader& reader, quaternion& value) {
            if (const std::optional<pfloat4> result = reader.Read<pfloat4>(); result) {
                const pfloat4 resultValue = result.value();
                value = math::normalize(quaternion(resultValue.x, resultValue.y, resultValue.z, resultValue.w));
            }
        }

        template <typename T>
        void ReadValue(BinaryReader& reader, AssetRef<T>& value) {
            if (const std::optional<AssetID> handleResult = reader.Read<AssetID>()) {
                value = AssetRef<T>(handleResult.value());
            }
        }

        template <typename T>
        static void ReadValue(BinaryReader& reader, SoftRef<T>& value) {
            if (const std::optional<AssetID> handleResult = reader.Read<AssetID>()) {
                value = SoftRef<T>(handleResult.value());
            }
        }

    private:
        HashMap<uint64_t, BufferView>& m_FieldsMap;
    };
}
