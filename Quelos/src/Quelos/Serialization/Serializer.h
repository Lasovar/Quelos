#pragma once

#include <variant>
#include <coroutine>

#include "QuelLexer.h"
#include "Quelos/Utility/Generator.h"

namespace Quelos::Serialization {
    // Document
    using PathID = uint64_t;

    struct FieldEvent {
        std::string_view Path;
        PathID ID = 0;

        FieldEvent() = default;

        FieldEvent(const std::string_view& path, const PathID id)
            : Path(path), ID(id) {
        }
    };

    struct ValueEvent {
        std::string_view Text;
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
        virtual void SetIndent(int indent) = 0;
        virtual void SetFormatting(QuelFormatting formatting) = 0;
    };

    class StringQuelWriter : public QuelWriter {
    public:
        StringQuelWriter() = delete;
        explicit StringQuelWriter(std::string& out) : m_Out(out) {}

        void Write(const ParserEvent& parserEvent) override;
        void SetIndent(const int indent) override { m_Indent = indent; }

        void SetFormatting(const QuelFormatting formatting) override { m_Formatting = formatting; }
    private:
        void WriteEscaped(std::string_view text) const;
        void WriteIndent();
        void NewLine();
        void CloseSectionHeader();

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
