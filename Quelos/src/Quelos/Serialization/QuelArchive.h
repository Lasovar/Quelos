#pragma once

#include <string_view>
#include <variant>
#include <charconv>

#include "Quelos/Math/Math.h"

#include <magic_enum/magic_enum.hpp>
#include <magic_enum/magic_enum_flags.hpp>

#include "Quelos/Core/Base.h"

#include "Quelos/Core/Log.h"
#include "Quelos/Renderer/Color.h"
#include "Quelos/Serialization/Serializer.h"

#include "Quelos/Utility/TupleLike.h"

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

namespace Quelos::Serialization {
    struct TextArchiveValue;

    struct QS_API TupleValue {
        Vec<size_t> Elements{Allocator::Temp};
    };

    struct QS_API ArrayValue {
        Vec<size_t> Elements{Allocator::Temp};
    };

    struct QS_API TextArchiveValue {
        std::variant<ValueEvent::ValueType, TupleValue, ArrayValue> Data;

        [[nodiscard]] bool IsScalar() const {
            return std::holds_alternative<ValueEvent::ValueType>(Data);
        }

        [[nodiscard]] bool IsTuple() const {
            return std::holds_alternative<TupleValue>(Data);
        }

        [[nodiscard]] bool IsArray() const {
            return std::holds_alternative<ArrayValue>(Data);
        }

        [[nodiscard]] const ValueEvent::ValueType& AsScalar() const {
            return std::get<ValueEvent::ValueType>(Data);
        }

        [[nodiscard]] const TupleValue& AsTuple() const {
            return std::get<TupleValue>(Data);
        }

        [[nodiscard]] const ArrayValue& AsArray() const {
            return std::get<ArrayValue>(Data);
        }
    };

    class QS_API QuelWriteArchive {
    public:
        static constexpr bool IsLoading = false;
        static constexpr bool IsSaving = true;

    public:
        explicit QuelWriteArchive(QuelWriter& writer)
            : m_Writer(writer) { }

        QuelWriteArchive(QuelWriter& writer, HashSet<std::string_view>* fieldToWrite)
            : m_Writer(writer), m_FieldsToWrite(fieldToWrite) { }

        void Section(const std::string_view name) const {
            m_Writer.Write(SectionEvent { name });
        }

        void CloseSection() const {
            m_Writer.CloseSection();
        }

        void Component(const std::string_view name) const {
            m_Writer.Write(ComponentEvent { name });
        }

        template <typename T>
        void Field(std::string_view name, T& value) {
            if (m_FieldsToWrite) {
                if (m_FieldsToWrite->find(name) != m_FieldsToWrite->end()) {
                    WriteValue(name, value);
                }
            }
            else {
                WriteValue(name, value);
            }
        }

        template <typename T>
        void FieldVector(const std::string& name, Vec<T>& vec) {
            WriteComplex(name, vec);
        }

        template <typename T>
        void Value(T& value) {
            WriteValue(value);
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
                    m_Writer.WriteField(name, UnquotedString{ enumName });
                }
                else {
                    auto enumName = magic_enum::enum_name(value);
                    m_Writer.WriteField(name, UnquotedString{ enumName });
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
        void WriteValue(T& value) {
            if constexpr (std::is_enum_v<T>) {
                if constexpr (magic_enum::detail::has_is_flags<T>::value) {
                    auto enumName = magic_enum::enum_flags_name(value);
                    m_Writer.Write(ValueEvent{UnquotedString{ enumName }});
                }
                else {
                    auto enumName = magic_enum::enum_name(value);
                    m_Writer.Write(ValueEvent { UnquotedString{ enumName } });
                }
            }
            else if constexpr (std::is_arithmetic_v<T>) {
                m_Writer.WriteValue(value);
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
                WriteComplex(value);
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

        template <typename T>
        void WriteComplex(const std::string_view name, HashMap<std::string, T>& map) {
            if (map.empty()) {
                return;
            }

            m_Writer.Write(FieldMapEvent { name });
            WriteComplex(map);
        }

        template <typename T>
        void WriteComplex(HashMap<std::string, T>& map) {
            if (map.empty()) {
                return;
            }

            for (auto& [name, value] : map) {
                m_Writer.Write(FieldEvent { name });
                WriteValue(value);
            }
        }

        void WriteComplex(const std::string_view name, const float2& v) const {
            m_Writer.WriteField(name, v);
        }

        void WriteComplex(const std::string_view name, const float3& v) const {
            m_Writer.WriteField(name, v);
        }

        void WriteComplex(const std::string_view name, const float4& v) const {
            m_Writer.WriteField(name, v);
        }

        void WriteComplex(const std::string_view name, const Color& v) const {
            m_Writer.WriteField(name, v);
        }

        void WriteComplex(const std::string_view name, const quaternion& q) const {
            m_Writer.WriteField(name, q);
        }

        void WriteComplex(const float2& v) const {
            m_Writer.WriteValue(v);
        }

        void WriteComplex(const float3& v) const {
            m_Writer.WriteValue(v);
        }

        void WriteComplex(const float4& v) const {
            m_Writer.WriteValue(v);
        }

        void WriteComplex(const Color& v) const {
            m_Writer.WriteValue(v);
        }

        void WriteComplex(const quaternion& q) const {
            m_Writer.WriteValue(q);
        }

        template <typename T>
        void WriteComplex(const std::string_view name, const AssetRef<T>& value) {
            m_Writer.WriteField(name, UnquotedString {value.GetAssetID().ToFormattedString() });
        }

        template <typename T>
        void WriteComplex(const std::string_view name, const SoftRef<T>& value) {
            m_Writer.WriteField(name, UnquotedString {value.GetAssetID().ToFormattedString() });
        }

        void WriteComplex(const std::string_view name, const AssetID& value) const {
            m_Writer.Write(FieldEvent { name });
            WriteComplex(value);
        }

        void WriteComplex(const AssetID& value) const {
            m_Writer.Write(ValueEvent { UnquotedString {value.ToFormattedString() } });
        }

        template <typename T>
        void WriteComplex(const AssetRef<T>& value) {
            m_Writer.Write(ValueEvent{ UnquotedString {value.GetAssetID().ToFormattedString() } });
        }

        template <typename T>
        void WriteComplex(const SoftRef<T>& value) {
            m_Writer.Write(ValueEvent { UnquotedString {value.GetAssetID().ToFormattedString() } });
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

        void WriteTupleElement(const float3& v) const {
            m_Writer.BeginTuple();
            m_Writer.WriteValue(v.x);
            m_Writer.WriteValue(v.y);
            m_Writer.WriteValue(v.z);
            m_Writer.EndTuple();
        }

    private:
        QuelWriter& m_Writer;
        HashSet<std::string_view>* m_FieldsToWrite = nullptr;
    };

    class QS_API QuelReadArchive {
    public:
        static constexpr bool IsLoading = true;
        static constexpr bool IsSaving = false;

    public:
        explicit QuelReadArchive(
            const Vec<Pair<std::string_view, size_t>>& fields,
            const Vec<TextArchiveValue>& valuePool
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

        template <VectorType T>
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
                if (!TryConvert(m_Pool[elems[i]], math::value_ptr(out)[i])) {
                    QS_CORE_ERROR_TAG("Serialization", "Couldn't deserialize a tuple value");
                    return false;
                }
            }

            return true;
        }

        bool TryConvert(const TextArchiveValue& src, float2& out) {
            return TryConvertTuple(src, out, 2);
        }

        bool TryConvert(const TextArchiveValue& src, float3& out) {
            return TryConvertTuple(src, out, 3);
        }

        bool TryConvert(const TextArchiveValue& src, float4& out) {
            return TryConvertTuple(src, out, 4);
        }

        bool TryConvert(const TextArchiveValue& src, Color& out) {
            return TryConvertTuple(src, static_cast<float4&>(out), 4);
        }

        bool TryConvert(const TextArchiveValue& src, quaternion& out) {
            return TryConvertTuple(src, out, 4);
        }

        template <typename T>
        bool TryConvert(const TextArchiveValue& src, AssetRef<T>& value) {
            if (src.IsScalar()) {
                const auto scalar = src.AsScalar();
                if (const auto* assetHandle = std::get_if<std::string_view>(&scalar)) {
                    if (const AssetID handle(*assetHandle); handle) {
                        value = AssetRef<T>(handle);
                    }

                    return true;
                }
            }

            return false;

            return false;
        }

        template <typename T>
        static bool TryConvert(const TextArchiveValue& src, SoftRef<T>& value) {
            if (src.IsScalar()) {
                const auto scalar = src.AsScalar();
                if (const auto* assetHandle = std::get_if<std::string_view>(&scalar)) {
                    value = SoftRef<T>(AssetID(*assetHandle));
                    return true;
                }
            }

            return false;
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
        const Vec<std::pair<std::string_view, size_t>>& m_Fields;
        const Vec<TextArchiveValue>& m_Pool;

        const TupleValue* m_CurrentTuple = nullptr;
        uint32_t m_TupleIndex = 0;
    };


    struct AutoTextArchiveValue;

    struct QS_API AutoTupleValue {
        Vec<AutoTextArchiveValue> Elements{Allocator::Persistent};
    };

    struct QS_API AutoArrayValue {
        Vec<AutoTextArchiveValue> Elements{Allocator::Persistent};
    };

    struct QS_API AutoTextArchiveValue {
        std::variant<ValueEvent::ValueType, AutoTupleValue, AutoArrayValue> Data;

        [[nodiscard]] bool IsScalar() const {
            return std::holds_alternative<ValueEvent::ValueType>(Data);
        }

        [[nodiscard]] bool IsTuple() const {
            return std::holds_alternative<AutoTupleValue>(Data);
        }

        [[nodiscard]] bool IsArray() const {
            return std::holds_alternative<AutoArrayValue>(Data);
        }

        [[nodiscard]] const ValueEvent::ValueType& AsScalar() const {
            return std::get<ValueEvent::ValueType>(Data);
        }

        [[nodiscard]] const AutoTupleValue& AsTuple() const {
            return std::get<AutoTupleValue>(Data);
        }

        [[nodiscard]] const AutoArrayValue& AsArray() const {
            return std::get<AutoArrayValue>(Data);
        }
    };

    struct FieldMap {
        SegmentedMap<std::string_view, AutoTextArchiveValue> Fields;
    };

    struct Component {
        SegmentedMap<std::string_view, AutoTextArchiveValue> Fields;
        SegmentedMap<std::string_view, FieldMap> FieldMaps;
    };

    struct Section {
        SegmentedMap<std::string_view, AutoTextArchiveValue> Fields;
        SegmentedMap<std::string_view, FieldMap> FieldMaps;
        SegmentedMap<std::string_view, Component> Components;
    };

    // TODO:
    inline Generator<std::string_view> FieldParts(const std::string_view path) {
        size_t start = 0;
        for (size_t i = 0; i <= path.size(); i++) {
            if (i != path.size() && path[i] != '.') {
                continue;
            }

            std::string_view part(path.data() + start, i - start);

            co_yield part;
            start = i + 1;
        }
    }

    class QS_API QuelAutoReadArchive {
    public:
        static constexpr bool IsLoading = true;
        static constexpr bool IsSaving = false;

    public:
        explicit QuelAutoReadArchive(QuelReader reader) {
            Serialization::Section* currentSection = nullptr;
            Serialization::Component* currentComponent = nullptr;
            Serialization::FieldMap* currentFieldMap = nullptr;
            std::string_view currentField;
            Vec<AutoTextArchiveValue*> containerStack{Allocator::Temp};

            for (auto& parseEvent : reader.Parse()) {
                std::visit(
                    Overloaded {
                        [&](const SectionEvent& event) {
                            currentSection = &m_Map.emplace(event.Name, Serialization::Section()).first->second;

                            currentField = "";
                            currentFieldMap = nullptr;
                            currentComponent = nullptr;
                        },
                        [&](const ComponentEvent& event) {
                            if (!currentSection) {
                                return;
                            }

                            currentComponent = &currentSection->Components.emplace(event.Name, Serialization::Component()).first->second;

                            currentField = "";
                            currentFieldMap = nullptr;
                        }, [&](const FieldMapEvent& event) {
                            if (!currentComponent && !currentSection) {
                                return;
                            }

                            SegmentedMap<std::string_view, Serialization::FieldMap>& fieldMap = currentComponent
                                    ? currentComponent->FieldMaps
                                    : currentSection->FieldMaps;

                            currentFieldMap = &fieldMap.emplace(event.Path, Serialization::FieldMap()).first->second;

                            currentField = "";
                        }, [&](const FieldEvent& event) {
                            if (!currentFieldMap && !currentComponent && !currentSection) {
                                return;
                            }

                            currentField = event.Path;
                        },
                        [&](const TupleBeginEvent& event) {
                            if (!containerStack.empty()) {
                                AutoTextArchiveValue& parent = *containerStack.back();

                                if (auto* tuple = std::get_if<AutoTupleValue>(&parent.Data)) {
                                    containerStack.push_back(&tuple->Elements.emplace_back(AutoTupleValue{}));
                                }
                                else if (auto* array = std::get_if<AutoArrayValue>(&parent.Data)) {
                                    containerStack.push_back(&array->Elements.emplace_back(AutoTupleValue{}));
                                }

                                return;
                            }

                            SegmentedMap<std::string_view, AutoTextArchiveValue>& fields = currentFieldMap
                                    ? currentFieldMap->Fields
                                    : currentComponent ? currentComponent->Fields : currentSection->Fields;

                            containerStack.emplace_back(&fields.try_emplace(currentField, AutoTupleValue{}).first->second);
                        },
                        [&](const TupleEndEvent& event) {
                            containerStack.pop_back();
                        },
                        [&](const ArrayBeginEvent& event) {
                            if (!containerStack.empty()) {
                                AutoTextArchiveValue& parent = *containerStack.back();

                                if (auto* tuple = std::get_if<AutoTupleValue>(&parent.Data)) {
                                    containerStack.push_back(&tuple->Elements.emplace_back(AutoTupleValue{}));
                                }
                                else if (auto* array = std::get_if<AutoArrayValue>(&parent.Data)) {
                                    containerStack.push_back(&array->Elements.emplace_back(AutoTupleValue{}));
                                }

                                return;
                            }

                            SegmentedMap<std::string_view, AutoTextArchiveValue>& fieldsMap = currentFieldMap
                                    ? currentFieldMap->Fields
                                    : currentComponent ? currentComponent->Fields : currentSection->Fields;

                            containerStack.emplace_back(&fieldsMap.try_emplace(currentField, AutoArrayValue{}).first->second);
                        },
                        [&](const ArrayEndEvent& event) {
                            containerStack.pop_back();
                        },
                        [&](const ValueEvent& event) {
                            if ((!currentFieldMap && !currentComponent && !currentSection) || currentField.empty()) {
                                return;
                            }

                            if (containerStack.empty()) {
                                SegmentedMap<std::string_view, AutoTextArchiveValue>& fieldsMap = currentFieldMap
                                        ? currentFieldMap->Fields
                                        : currentComponent ? currentComponent->Fields : currentSection->Fields;

                                fieldsMap.emplace(currentField, event.Value);
                            } else {
                                AutoTextArchiveValue& parent = *containerStack.back();

                                if (auto* tuple = std::get_if<AutoTupleValue>(&parent.Data)) {
                                    tuple->Elements.emplace_back(event.Value);
                                }
                                else if (auto* array = std::get_if<AutoArrayValue>(&parent.Data)) {
                                    array->Elements.emplace_back(event.Value);
                                }
                            }
                        },
                        [](const ParseError& parseError) {
                            QS_CORE_ERROR_TAG(
                                "QuelAutoReadArchive",
                                "Failed to parse at line {}: {}",
                                parseError.Line, parseError.Message
                            );
                        }
                    },
                    parseEvent
                );
            }
        }

        void Section(const std::string_view name) {
            const auto it = m_Map.find(name);
            if (it == m_Map.end()) {
                return;
            }

            m_CurrentFields = &it->second.Fields;
            m_CurrentSection = &it->second;

            m_CurrentFieldMap = nullptr;
            m_CurrentComponent = nullptr;
        }

        static void CloseSection() {}

        void Component(const std::string_view name) {
            if (!m_CurrentSection) {
                return;
            }

            const auto it = m_CurrentSection->Components.find(name);
            if (it == m_CurrentSection->Components.end()) {
                return;
            }

            m_CurrentComponent = &it->second;
            m_CurrentFields = &m_CurrentComponent->Fields;

            m_CurrentFields = nullptr;
        }

        void FieldMap(const std::string_view name) {
            if (!m_CurrentComponent && !m_CurrentSection) {
                return;
            }

            SegmentedMap<std::string_view, Serialization::FieldMap>& fieldsMaps = m_CurrentComponent
                    ? m_CurrentComponent->FieldMaps
                    : m_CurrentSection->FieldMaps;

            const auto it = fieldsMaps.find(name);
            if (it == fieldsMaps.end()) {
                return;
            }

            m_CurrentFieldMap = &it->second;
        }

        template <typename T>
        void Field(const std::string_view name, T& value) {
            if (!m_CurrentFields) {
                return;
            }

            const auto it = m_CurrentFields->find(name);
            if (it == m_CurrentFields->end()) {
                return;
            }

            TryConvert(it->second, value);
        }

        template <typename T>
        void Field(const std::string_view name, HashMap<std::string, T>& map) {
            if (!m_CurrentComponent && !m_CurrentSection) {
                return;
            }

            SegmentedMap<std::string_view, Serialization::FieldMap>& fieldMap = m_CurrentComponent
                                                                          ? m_CurrentComponent->FieldMaps
                                                                          : m_CurrentSection->FieldMaps;
            const auto it = fieldMap.find(name);
            if (it == fieldMap.end()) {
                return;
            }

            for (auto& [key, value] : it->second.Fields) {
                 TryConvert(value, map[std::string(key)]);
            }
        }

        template <typename T>
        void FieldVector(const std::string& name, std::vector<T>& vec) {
            Field(name, vec);
        }

        template <typename T>
        void Value(T& value) {
            TryConvert(m_CurrentTuple->Elements[m_TupleIndex++], value);
        }

        static void BeginTuple() { }
        static void BeginTupleField(const std::string_view) { }
        static void EndTuple() { }

    private:
        template <typename T>
        bool TryConvert(const AutoTextArchiveValue& src, T& out) {
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

                const AutoTupleValue* prevTuple = m_CurrentTuple;
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
        Option<T> ConvertScalar(const ValueEvent::ValueType& v) {
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

                        return None;
                    }
                    else {
                        return None;
                    }
                },
                v
            );
        }

        template <VectorType T>
        bool TryConvertTuple(const AutoTextArchiveValue& src, T& out, const size_t expectedSize) {
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
                if (!TryConvert(elems[i], math::value_ptr(out)[i])) {
                    QS_CORE_ERROR_TAG("Serialization", "Couldn't deserialize a tuple value");
                    return false;
                }
            }

            return true;
        }

        bool TryConvert(const AutoTextArchiveValue& src, float2& out) {
            return TryConvertTuple(src, out, 2);
        }

        bool TryConvert(const AutoTextArchiveValue& src, float3& out) {
            return TryConvertTuple(src, out, 3);
        }

        bool TryConvert(const AutoTextArchiveValue& src, float4& out) {
            return TryConvertTuple(src, out, 4);
        }

        bool TryConvert(const AutoTextArchiveValue& src, Color& out) {
            return TryConvertTuple(src, static_cast<float4&>(out), 4);
        }

        bool TryConvert(const AutoTextArchiveValue& src, quaternion& out) {
            return TryConvertTuple(src, out, 4);
        }

        static bool TryConvert(const AutoTextArchiveValue& src, GUID128& out) {
            const auto& scalar = src.AsScalar();

            if (const std::string_view* str = std::get_if<std::string_view>(&scalar)) {
                out = *str;
                return true;
            }

            return false;
        }

        template <typename T>
        bool TryConvert(const AutoTextArchiveValue& src, AssetRef<T>& value) {
            if (src.IsScalar()) {
                const auto scalar = src.AsScalar();
                if (const auto* assetHandle = std::get_if<std::string_view>(&scalar)) {
                    if (const AssetID handle(*assetHandle); handle) {
                        value = AssetRef<T>(handle);
                    }

                    return true;
                }
            }

            return false;

            return false;
        }

        template <typename T>
        static bool TryConvert(const AutoTextArchiveValue& src, SoftRef<T>& value) {
            if (src.IsScalar()) {
                const auto scalar = src.AsScalar();
                if (const auto* assetHandle = std::get_if<std::string_view>(&scalar)) {
                    value = SoftRef<T>(AssetID(*assetHandle));
                    return true;
                }
            }

            return false;
        }

        template <typename T>
        bool TryConvert(const AutoTextArchiveValue& src, std::vector<T>& out) {
            if (!src.IsArray()) {
                QS_CORE_ERROR_TAG("Serialization", "Expected array");
                return false;
            }

            const auto& elems = src.AsArray().Elements;

            out.clear();
            out.reserve(elems.size());

            for (const auto& elem : elems) {
                T value{};
                if (!TryConvert(elem, value)) {
                    QS_CORE_ERROR_TAG("Serialization", "Couldn't deserialize an array value");
                    return false;
                }

                out.push_back(std::move(value));
            }

            return true;
        }

    private:
        Serialization::Section* m_CurrentSection = nullptr;
        Serialization::Component* m_CurrentComponent = nullptr;
        Serialization::FieldMap* m_CurrentFieldMap = nullptr;

        SegmentedMap<std::string_view, AutoTextArchiveValue>* m_CurrentFields = nullptr;
        SegmentedMap<std::string_view, Serialization::Section> m_Map;

        const AutoTupleValue* m_CurrentTuple = nullptr;
        uint32_t m_TupleIndex = 0;
    };
}
