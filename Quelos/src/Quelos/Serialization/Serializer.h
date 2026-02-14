#pragma once

#include <variant>
#include <coroutine>

#include "Lexer.h"
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

    struct TupleBeginEvent { };
    struct TupleEndEvent { };

    struct ArrayBeginEvent { };
    struct ArrayEndEvent { };

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

    class Parser {
    public:
        Parser() = delete;

        explicit Parser(const std::string_view input)
            : m_Lexer(input)
        {
        }

        Generator<ParserEvent> Parse();

    private:
        Generator<ParserEvent> ParseValue();

    private:
        Lexer m_Lexer;
    };
}
