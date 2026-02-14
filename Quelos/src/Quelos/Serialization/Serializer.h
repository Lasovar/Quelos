#pragma once

#include <variant>
#include <coroutine>

#include "Quelos/Utility/Generator.h"

namespace Quelos::Serialization {
    // Document
    using PathID = uint64_t;

    struct ValueEvent {
        std::string_view Text;
    };

    struct FieldEvent {
        std::string_view Path;
        ValueEvent Value;
        PathID ID = 0;

        FieldEvent() = default;
        FieldEvent(const std::string_view& path, const std::string_view& value, const PathID id)
            : Path(path), Value(value), ID(id)
        {}
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

    using ParserEvent = std::variant<SectionEvent, ComponentEvent, FieldEvent, ParseError>;

    // Core

    class Parser {
    public:
        Parser() = delete;
        explicit Parser(const std::string_view input) : m_Input(input) {}
        Generator<ParserEvent> Parse();

    private:
        std::string_view m_Input;
        size_t m_LineNumber = 0;
    };
}
