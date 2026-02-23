#pragma once

#include <string_view>
#include <variant>

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"

#include <magic_enum/magic_enum.hpp>
#include <magic_enum/magic_enum_flags.hpp>

#include "Quelos/Serialization/Serializer.h"

namespace Quelos::Serialization {
    class QuelReadArchive;
    class QuelWriteArchive;
}

template <typename TComponent, typename TArchive>
concept SerializableWith = requires(TArchive& archive, TComponent& component) {
    TComponent::Serialize(archive, component);
};

template <typename T>
constexpr bool IsSerializable = SerializableWith<T, Quelos::Serialization::BinaryWriteArchive>
    && SerializableWith<T, Quelos::Serialization::BinaryReadArchive>
    && SerializableWith<T, Quelos::Serialization::QuelWriteArchive>
    && SerializableWith<T, Quelos::Serialization::QuelReadArchive>;

template <typename T>
concept TupleLike = requires(T t) {
    t[0];
};

namespace Quelos::Serialization {
    struct TextArchiveValue;

    struct TupleValue {
        std::vector<size_t> Elements;
    };

    struct ArrayValue {
        std::vector<size_t> Elements;
    };

    struct TextArchiveValue {
        std::variant<ValueEvent::ValueType, TupleValue, ArrayValue> Data;

        bool IsScalar() const {
            return std::holds_alternative<ValueEvent::ValueType>(Data);
        }

        bool IsTuple() const {
            return std::holds_alternative<TupleValue>(Data);
        }

        bool IsArray() const {
            return std::holds_alternative<ArrayValue>(Data);
        }

        const ValueEvent::ValueType& AsScalar() const {
            return std::get<ValueEvent::ValueType>(Data);
        }

        const TupleValue& AsTuple() const {
            return std::get<TupleValue>(Data);
        }

        const ArrayValue& AsArray() const {
            return std::get<ArrayValue>(Data);
        }
    };

    class QuelWriteArchive {
    public:
        static constexpr bool IsLoading = false;
        static constexpr bool IsSaving = true;

    public:
        explicit QuelWriteArchive(QuelWriter& writer)
            : m_Writer(writer) { }

        template <typename T>
        void Field(std::string_view name, T& value) {
            WriteValue(name, value);
        }

        template <typename T>
        void FieldVector(const std::string& name, std::vector<T>& vec) {
            WriteComplex(name, vec);
        }

        template <typename T>
        void Value(T& value) {
            m_Writer.WriteValue(value);
        }

        void BeginTuple() const {
            m_Writer.BeginTuple();
        }

        void BeginTupleField(const std::string_view name) const {
            m_Writer.Write(FieldEvent{ name });
            BeginTuple();
        }

        void EndTuple() const {
            m_Writer.EndTuple();
        }

    private:
        template <typename T>
        void WriteValue(std::string_view name, T& value) {
            if constexpr (std::is_enum_v<T>) {
                if constexpr (magic_enum::detail::has_is_flags<T>::value) {
                    auto enumName = magic_enum::enum_flags_name(value);
                    m_Writer.WriteField(name, std::string_view{ enumName });
                }
                else {
                    auto enumName = magic_enum::enum_name(value);
                    m_Writer.WriteField(name, std::string_view{ enumName });
                }
            }
            else if constexpr (std::is_arithmetic_v<T>) {
                m_Writer.WriteField(name, value);
            }
            else if constexpr (std::is_same_v<T, std::string>) {
                m_Writer.WriteField(name, value);
            }
            else if constexpr (SerializableWith<T, QuelWriteArchive>) {
                m_Writer.BeginTupleField(name);

                T copy = value;
                T::Serialize(*this, copy);

                m_Writer.EndTuple();
            }
            else {
                WriteComplex(name, value);
            }
        }

        template <typename T>
        void WriteComplex(const std::string_view name, const std::vector<T>& vec) {
            m_Writer.BeginArrayField(name);

            for (const auto& elem : vec) {
                WriteElement(elem);
            }

            m_Writer.EndArray();
        }

        void WriteComplex(const std::string_view name, const glm::vec2& v) const {
            m_Writer.BeginTupleField(name);
            m_Writer.WriteValue(v.x);
            m_Writer.WriteValue(v.y);
            m_Writer.EndTuple();
        }

        void WriteComplex(const std::string_view name, const glm::vec3& v) const {
            m_Writer.BeginTupleField(name);
            m_Writer.WriteValue(v.x);
            m_Writer.WriteValue(v.y);
            m_Writer.WriteValue(v.z);
            m_Writer.EndTuple();
        }

        void WriteComplex(const std::string_view name, const glm::vec4& v) const {
            m_Writer.BeginTupleField(name);
            m_Writer.WriteValue(v.x);
            m_Writer.WriteValue(v.y);
            m_Writer.WriteValue(v.z);
            m_Writer.WriteValue(v.w);
            m_Writer.EndTuple();
        }

        void WriteComplex(const std::string_view name, const glm::quat& q) const {
            m_Writer.BeginTupleField(name);
            m_Writer.WriteValue(q.w);
            m_Writer.WriteValue(q.x);
            m_Writer.WriteValue(q.y);
            m_Writer.WriteValue(q.z);
            m_Writer.EndTuple();
        }

        template <typename T>
        void WriteElement(const T& value) {
            if constexpr (std::is_arithmetic_v<T> || std::is_enum_v<T>) {
                if constexpr (std::is_unsigned_v<T>) {
                    m_Writer.WriteValue(static_cast<uint64_t>(value));
                }
                else {
                    m_Writer.WriteValue(static_cast<int64_t>(value));
                }
            }
            else if constexpr (std::is_same_v<T, std::string>) {
                m_Writer.WriteValue(value);
            }
            else if constexpr (SerializableWith<T, QuelWriteArchive>) {
                m_Writer.BeginTuple();

                T copy = value;
                T::Serialize(*this, copy);

                m_Writer.EndTuple();
            }
            else {
                WriteTupleElement(value);
            }
        }

        void WriteTupleElement(const glm::vec3& v) const {
            m_Writer.BeginTuple();
            m_Writer.WriteValue(v.x);
            m_Writer.WriteValue(v.y);
            m_Writer.WriteValue(v.z);
            m_Writer.EndTuple();
        }

    private:
        QuelWriter& m_Writer;
    };

    class QuelReadArchive {
    public:
        static constexpr bool IsLoading = true;
        static constexpr bool IsSaving = false;

    public:
        explicit QuelReadArchive(
            const std::vector<std::pair<std::string_view, size_t>>& fields,
            const std::vector<TextArchiveValue>& valuePool
        )
            : m_Fields(fields), m_Pool(valuePool) { }

        template <typename T>
        void Field(const std::string& name, T& value) {
            auto it = std::find_if(
                m_Fields.begin(),
                m_Fields.end(),
                [&](const auto& pair) { return pair.first == name; }
            );

            if (it == m_Fields.end()) {
                return;
            }

            const TextArchiveValue& val = m_Pool[it->second];
            TryConvert(val, value);
        }

        template <typename T>
        void FieldVector(const std::string& name, std::vector<T>& vec) {
            Field(name, vec);
        }

        template <typename T>
        void Value(T& value) {
            TryConvert(m_Pool[m_CurrentTuple->Elements[m_TupleIndex++]], value);
        }

        static void BeginTuple() { }
        static void BeginTupleField(const std::string_view) { }
        static void EndTuple() { }

    private:
        template <typename T>
        bool TryConvert(const TextArchiveValue& src, T& out) {
            if constexpr (std::is_enum_v<T>) {
                if (!src.IsScalar()) {
                    return false;
                }

                const auto& scalar = src.AsScalar();

                if (const auto str = std::get_if<std::string_view>(&scalar)) {
                    if constexpr (magic_enum::detail::has_is_flags<T>::value) {
                        auto result = magic_enum::enum_flags_cast<T>(*str);
                        if (result) {
                            out = result.value();
                            return true;
                        }
                    }
                    else {
                        auto result = magic_enum::enum_cast<T>(*str);
                        if (result) {
                            out = result.value();
                            return true;
                        }
                    }
                }

                QS_CORE_ERROR_TAG("Serialization", "Invalid enum value");
                return false;
            }
            else if constexpr (std::is_arithmetic_v<T>) {
                if (!src.IsScalar()) {
                    QS_CORE_ERROR_TAG("Serialization", "Expected scalar");
                    return false;
                }

                std::optional<T> result = ConvertScalar<T>(src.AsScalar());
                if (result) {
                    out = *result;
                    return true;
                }

                QS_CORE_ERROR_TAG("Serialization", "Couldn't convert scalar");
                return false;
            }
            else if constexpr (std::is_same_v<T, std::string>) {
                if (!src.IsScalar()) {
                    QS_CORE_ERROR_TAG("Serialization", "Expected string");
                    return false;
                }

                const auto& scalar = src.AsScalar();

                if (const std::string_view* str = std::get_if<std::string_view>(&scalar)) {
                    out = *str;
                    return true;
                }

                QS_CORE_ERROR_TAG("Serialization", "Scalar is not a string");
                return false;
            }
            else if constexpr (IsSerializable<T>) {
                if (!src.IsTuple()) {
                    return false;
                }

                const TupleValue* prevTuple = m_CurrentTuple;
                const size_t prevIndex = m_TupleIndex;

                m_CurrentTuple = &src.AsTuple();
                m_TupleIndex = 0;

                T::Serialize(*this, out);

                m_CurrentTuple = prevTuple;
                m_TupleIndex = prevIndex;

                return true;
            }
            else {
                static_assert(sizeof(T) == 0, "Unsupported type");
            }

            return false;
        }

        template <typename T>
        std::optional<T> ConvertScalar(const ValueEvent::ValueType& v) {
            return std::visit(
                []<typename TValue>(TValue&& value) -> std::optional<T> {
                    using V = std::decay_t<TValue>;

                    if constexpr (std::is_arithmetic_v<V> && std::is_arithmetic_v<T>) {
                        return static_cast<T>(value);
                    }
                    else if constexpr (std::is_enum_v<T> && std::is_arithmetic_v<V>) {
                        return static_cast<T>(value);
                    }
                    else if constexpr (std::is_same_v<V, std::string_view> && std::is_arithmetic_v<T>) {
                        T result{};
                        auto [ptr, ec] = std::from_chars(
                            value.data(),
                            value.data() + value.size(),
                            result
                        );

                        if (ec == std::errc()) {
                            return result;
                        }

                        return std::nullopt;
                    }
                    else {
                        return std::nullopt;
                    }
                },
                v
            );
        }

        template <TupleLike T>
        bool TryConvertTuple(const TextArchiveValue& src, T& out, const size_t expectedSize) {
            if (!src.IsTuple()) {
                QS_CORE_ERROR_TAG("Serialization", "Expected tuple");
                return false;
            }

            const auto& elems = src.AsTuple().Elements;

            if (elems.size() != expectedSize) {
                QS_CORE_ERROR_TAG("Serialization", "Invalid tuple size");
                return false;
            }

            for (size_t i = 0; i < expectedSize; ++i) {
                if (!TryConvert(m_Pool[elems[i]], out[i])) {
                    QS_CORE_ERROR_TAG("Serialization", "Couldn't deserialize a tuple value");
                    return false;
                }
            }

            return true;
        }

        bool TryConvert(const TextArchiveValue& src, glm::vec2& out) {
            return TryConvertTuple(src, out, 2);
        }

        bool TryConvert(const TextArchiveValue& src, glm::vec3& out) {
            return TryConvertTuple(src, out, 3);
        }

        bool TryConvert(const TextArchiveValue& src, glm::vec4& out) {
            return TryConvertTuple(src, out, 4);
        }

        bool TryConvert(const TextArchiveValue& src, glm::quat& out) {
            return TryConvertTuple(src, out, 4);
        }

        template <typename T>
        bool TryConvert(const TextArchiveValue& src, std::vector<T>& out) {
            if (!src.IsArray()) {
                QS_CORE_ERROR_TAG("Serialization", "Expected array");
                return false;
            }

            const auto& elems = src.AsArray().Elements;

            out.clear();
            out.reserve(elems.size());

            for (const auto& elem : elems) {
                T value{};
                if (!TryConvert(m_Pool[elem], value)) {
                    QS_CORE_ERROR_TAG("Serialization", "Couldn't deserialize an array value");
                    return false;
                }

                out.push_back(std::move(value));
            }

            return true;
        }

    private:
        const std::vector<std::pair<std::string_view, size_t>>& m_Fields;
        const std::vector<TextArchiveValue>& m_Pool;

        const TupleValue* m_CurrentTuple = nullptr;
        uint32_t m_TupleIndex = 0;
    };
}
