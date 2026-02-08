#include "qspch.h"
#include "Serializer.h"

#include "xxhash.h"

namespace Quelos::Serialization {
    namespace Utils {
        inline PathID GetPathID(const std::string_view path) {
            return XXH3_64bits(path.data(), path.size());
        }

        inline std::string_view Trim(std::string_view stringView) {
            while (!stringView.empty() && std::isspace(static_cast<unsigned char>(stringView.front()))) {
                stringView.remove_prefix(1);
            }

            while (std::isspace(static_cast<unsigned char>(stringView.back())) && !stringView.empty()) {
                stringView.remove_suffix(1);
            }

            // ReSharper disable once CppDFALocalValueEscapesFunction
            return stringView;
        }

        inline bool StartsWith(const std::string_view s, const std::string_view p) {
            return s.substr(0, p.size()) == p;
        }

        static std::string UnescapeString(const std::string_view input) {
            std::string result;
            result.reserve(input.size());

            for (size_t i = 0; i < input.size(); ++i) {
                if (const char c = input[i]; c == '\\' && i + 1 < input.size()) {
                    switch (const char next = input[++i]) {
                    case '"':  result.push_back('"');  break;
                    case '\\': result.push_back('\\'); break;
                    case 'n':  result.push_back('\n'); break;
                    case 't':  result.push_back('\t'); break;
                    default:
                        result.push_back('\\');
                        result.push_back(next);
                        break;
                    }
                }
                else {
                    result.push_back(c);
                }
            }

            return result;
        }
    }

    static std::variant<std::string, ParseError>
    ParseValueToken(const std::string_view text, size_t& position, const size_t lineNumber) {
        if (position >= text.size()) {
            return std::string();
        }

        // Quoted text
        if (text[position] == '"') {
            position++;
            const size_t start = position;

            // Parsing special characters
            while (position < text.size() && text[position] != '"') {
                if (text[position] == '\\' && position + 1 < text.size()) {
                    position += 2;
                }
                else {
                    position++;
                }
            }

            if (position >= text.size()) {
                return ParseError{lineNumber, "Unterminated string literal"};
            }

            const std::string_view raw = text.substr(start, position - start);
            position++; // skip closing quote

            return Utils::UnescapeString(raw);
        }

        // Unquoted value
        const size_t start = position;

        while (position < text.size() && !std::isspace(static_cast<unsigned char>(text[position]))) {
            position++;
        }

        return std::string(text.substr(start, position - start));
    }

    static std::variant<Section, ParseError> ParseBracketSection(const std::string_view line, const size_t lineNumber) {
        if (line.size() < 2 || line.front() != '[' || line.back() != ']') {
            return ParseError{lineNumber, "Malformed header: missing '[' or ']'"};
        }

        // Remove brackets
        const std::string_view inside = Utils::Trim(line.substr(1, line.size() - 2));

        if (inside.empty()) {
            return ParseError{lineNumber, "Empty header"};
        }

        Section section;

        size_t position = 0;
        while (position < inside.size() && !std::isspace(static_cast<unsigned char>(inside[position]))) {
            position++;
        }

        section.Name = std::string(inside.substr(0, position));

        while (position < inside.size()) {
            // Skip whitespace
            while (position < inside.size() && std::isspace(static_cast<unsigned char>(inside[position]))) {
                position++;
            }

            if (position >= inside.size()) {
                break;
            }

            // Read key
            const size_t keyStart = position;
            while (
                position < inside.size()
                && inside[position] != '='
                && !std::isspace(static_cast<unsigned char>(inside[position]))
            ) {
                position++;
            }

            if (position >= inside.size() || inside[position] != '=') {
                return ParseError{lineNumber, "Expected '=' after key in header"};
            }

            std::string_view key = inside.substr(keyStart, position - keyStart);
            position++; // skip '='

            std::variant<std::string, ParseError> valueResult = ParseValueToken(inside, position, lineNumber);

            if (std::holds_alternative<ParseError>(valueResult)) {
                return std::get<ParseError>(valueResult);
            }

            section.Fields.emplace_back(std::string(key), Utils::GetPathID(key), std::get<std::string>(valueResult));
        }

        return section;
    }

    std::variant<Document, ParseError> Parser::Parse(std::string_view input) {
        m_Document = {};
        m_CurrentSection = nullptr;

        size_t lineNumber = 1;

        while (!input.empty()) {
            const std::size_t position = input.find('\n');
            const std::string_view line = position == std::string_view::npos ? input : input.substr(0, position);
            if (line.empty()) {
                input.remove_prefix(position + 1);
                lineNumber++;
                continue;
            }

            if (std::optional<ParseError> error = ParseLine(Utils::Trim(line), lineNumber)) {
                return error.value();
            }

            if (position == std::string_view::npos) {
                break;
            }

            input.remove_prefix(position + 1);
            lineNumber++;
        }

        return m_Document;
    }

    std::optional<ParseError> Parser::ParseLine(const std::string_view line, const size_t lineNumber) {
        if (line.empty() || Utils::StartsWith(line, "//")) {
            return std::nullopt;
        }

        if (line.front() == '[') {
            return ParseSection(line, lineNumber);
        }

        if (line.front() == '@') {
            return ParseComponent(line, lineNumber);
        }

        return ParseField(line, lineNumber);
    }

    std::optional<ParseError> Parser::ParseSection(const std::string_view line, const size_t lineNumber) {
        const std::variant<Section, ParseError> result = ParseBracketSection(line, lineNumber);
        if (std::holds_alternative<ParseError>(result)) {
            return std::get<ParseError>(result);
        }

        m_Document.Sections.push_back(std::get<Section>(result));
        m_CurrentComponent = nullptr;
        m_CurrentSection = &m_Document.Sections.back();

        return std::nullopt;
    }

    std::optional<ParseError> Parser::ParseComponent(const std::string_view line, const size_t lineNumber) {
        if (!m_CurrentSection) {
            return ParseError{lineNumber, "Component outside of section"};
        }

        m_CurrentSection->Components.push_back({});
        m_CurrentComponent = &m_CurrentSection->Components.back();
        m_CurrentComponent->Name = Utils::Trim(line.substr(1, line.size() - 1));

        return std::nullopt;
    }

    std::optional<ParseError> Parser::ParseField(std::string_view line, const size_t lineNumber) const {
        if (!m_CurrentSection) {
            return ParseError{lineNumber, "Field outside of section"};
        }

        const std::size_t eq = line.find('=');
        if (eq == std::string_view::npos) {
            return ParseError{lineNumber, "Expected '=' in field"};
        }

        const std::string_view key = Utils::Trim(line.substr(0, eq));
        const std::string_view rest = Utils::Trim(line.substr(eq + 1));

        size_t position = 0;

        std::variant<std::string, ParseError> valueResult = ParseValueToken(rest, position, lineNumber);
        if (std::holds_alternative<ParseError>(valueResult)) {
            return std::get<ParseError>(valueResult);
        }

        Field field(std::string(key), Utils::GetPathID(key), std::get<std::string>(valueResult));

        if (m_CurrentComponent) {
            m_CurrentComponent->Fields.push_back(std::move(field));
        }
        else {
            m_CurrentSection->Fields.push_back(std::move(field));
        }

        return std::nullopt;
    }
}
