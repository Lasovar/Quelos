#pragma once

#include <variant>

namespace Quelos::Serialization {
    // Document
    using PathID = uint64_t;

    constexpr PathID GetPathID(std::string_view path);

    struct Value {
        std::string Text;
    };

    struct Field {
        std::string Path;
        PathID ID;
        Value Value;

        Field() = default;
        Field(std::string path, const PathID id, std::string value)
            : Path(std::move(path)), ID(id), Value(std::move(value))
        {}
    };

    struct Component {
        std::string Name;
        std::vector<Field> Fields;
    };

    struct Section {
        std::string Name;
        std::vector<Field> Fields;
        std::vector<Component> Components;
    };

    struct Document {
        std::vector<Section> Sections;
    };

    // Error handling

    struct ParseError {
        size_t Line = 0;
        std::string Message;
    };

    // Core

    class Parser {
    public:
        std::variant<Document, ParseError> Parse(std::string_view input);

    private:
        std::optional<ParseError> ParseLine(std::string_view line, size_t lineNumber);
        std::optional<ParseError> ParseSection(std::string_view line, size_t lineNumber);
        std::optional<ParseError> ParseComponent(std::string_view line, size_t lineNumber);
        std::optional<ParseError> ParseField(std::string_view line, size_t lineNumber) const;

    private:
        Document m_Document;
        Section* m_CurrentSection = nullptr;
        Component* m_CurrentComponent = nullptr;
    };
}
