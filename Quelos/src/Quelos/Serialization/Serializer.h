#pragma once

#include <variant>

#include "glm/glm.hpp"

#include "BinaryWriter.h"
#include "QuelLexer.h"
#include "Quelos/Utility/Generator.h"

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
    // Document
    using PathID = uint64_t;

    struct FieldEvent {
        std::string_view Path;

        FieldEvent() = default;

        explicit FieldEvent(const std::string_view& path)
            : Path(path) {
        }
    };

    struct ValueEvent {
        using ValueType = std::variant<
            char,
            float,
            double,
            int64_t,
            uint64_t,
            bool,
            std::string_view
        >;

        ValueType Value;
    };

    struct ComponentEvent {
        std::string_view Name;
    };

    struct SectionEvent {
        std::string_view Name;
    };

    // Error handling
    struct ParseError {
        size_t Line = 0;
        std::string Message;
    };

    struct TupleBeginEvent {
    };

    struct TupleEndEvent {
    };

    struct ArrayBeginEvent {
    };

    struct ArrayEndEvent {
    };

    using ParserEvent = std::variant<
        SectionEvent,
        ComponentEvent,
        FieldEvent,
        ValueEvent,
        TupleBeginEvent,
        TupleEndEvent,
        ArrayBeginEvent,
        ArrayEndEvent,
        ParseError
    >;

    // Core

    class QuelReader {
    public:
        QuelReader() = delete;

        explicit QuelReader(const std::string_view input)
            : m_Lexer(input) {
        }

        Generator<ParserEvent> Parse();

    private:
        Generator<ParserEvent> ParseValue();

    private:
        QuelLexer m_Lexer;
    };

    enum class QuelFormatting {
        None,
        Indented,
    };

    class QuelWriter {
    public:
        virtual ~QuelWriter() = default;

        virtual void Write(const ParserEvent& ev) = 0;

        void WriteValue(uint32_t value);
        void WriteValue(uint64_t value);
        void WriteValue(int32_t value);
        void WriteValue(int64_t value);
        void WriteValue(float value);
        void WriteValue(double value);

        inline void WriteValue(glm::vec2 value);
        inline void WriteValue(glm::vec3 value);
        inline void WriteValue(glm::vec4 value);
        inline void WriteValue(glm::quat value);

        void WriteField(std::string_view field, std::string_view value);
        void WriteField(std::string_view field, bool value);
        void WriteField(std::string_view field, int64_t value);
        void WriteField(std::string_view field, uint64_t value);
        void WriteField(std::string_view field, uint32_t value);
        void WriteField(std::string_view field, float value);
        void WriteField(std::string_view field, double value);
        inline void WriteField(std::string_view field, glm::vec2 value);
        inline void WriteField(std::string_view field, glm::vec3 value);
        inline void WriteField(std::string_view field, glm::quat value);

        void BeginTupleField(std::string_view name);
        void BeginTuple();
        void EndTuple();

        void BeginArrayField(std::string_view name);
        void BeginArray();
        void EndArray();

        virtual void SetIndent(int indent) = 0;
        virtual void SetFormatting(QuelFormatting formatting) = 0;
    };

    class StringQuelWriter : public QuelWriter {
    public:
        StringQuelWriter() = delete;

        explicit StringQuelWriter(std::string& out) : m_Out(out) {
        }

        void Write(const ParserEvent& parserEvent) override;

        void SetIndent(const int indent) override { m_Indent = indent; }
        void SetFormatting(const QuelFormatting formatting) override { m_Formatting = formatting; }

    private:
        void WriteEscaped(std::string_view text) const;
        void WriteIndent();
        void NewLine();
        void CloseSectionHeader();

        template <typename T>
        void AppendNumber(T value) {
            char buffer[64];
            auto [ptr, errorCode] = std::to_chars(buffer, buffer + sizeof(buffer), value);

            if (errorCode == std::errc()) {
                m_Out.append(buffer, ptr);
            }
            else {
                m_Out += "0";
            }
        }

    private:
        std::string& m_Out;

        int m_Indent = 0;
        QuelFormatting m_Formatting = QuelFormatting::Indented;

        bool m_AtLineStart = true;
        bool m_InSectionHeader = false;
        bool m_InArray = false;
        bool m_InTuple = false;
    };
}
