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
    }

    using Token = std::variant<std::string_view, std::string, ParseError>;

    static Token ParseValueToken(const std::string_view text, size_t& position, const size_t lineNumber) {
        if (position >= text.size()) {
            return std::string_view{};
        }

        // Quoted
        if (text[position] == '"') {
            position++; // Skip opening quote
            const size_t start = position;

            bool hasEscape = false;
            while (position < text.size() && text[position] != '"') {
                if (text[position] == '\\' && position + 1 < text.size()) {
                    hasEscape = true;
                    position += 2;
                } else {
                    position++;
                }
            }

            if (position >= text.size()) {
                return ParseError{ lineNumber, "Unterminated string literal" };
            }

            std::string_view raw = text.substr(start, position - start);
            position++; // Skip closing quote

            if (!hasEscape) {
                return raw;
            }

            std::string result;
            result.reserve(raw.size());

            for (size_t i = 0; i < raw.size(); i++) {
                const char c = raw[i];

                if (c == '\\' && i + 1 < raw.size()) {
                    const char escaped = raw[++i];

                    switch (escaped)
                    {
                    case '"':  result.push_back('"');  break;
                    case '\\': result.push_back('\\'); break;
                    case 'n':  result.push_back('\n'); break;
                    case 't':  result.push_back('\t'); break;

                    default:
                        result.push_back('\\');
                        result.push_back(escaped);
                        break;
                    }
                }
                else {
                    result.push_back(c);
                }
            }

            return result;
        }

        // Unquoted
        const size_t start = position;

        while (position < text.size() && !std::isspace(static_cast<unsigned char>(text[position]))) {
            position++;
        }

        return text.substr(start, position - start);
    }

    Generator<ParserEvent> Parser::Parse() {
        m_LineNumber = 1;

        while (!m_Input.empty()) {
            const size_t pos = m_Input.find('\n');
            std::string_view line = pos == std::string_view::npos
                ? m_Input
                : m_Input.substr(0, pos);

            m_Input.remove_prefix(
                pos == std::string_view::npos ? m_Input.size() : pos + 1
            );

            if (line.empty() || Utils::StartsWith(line, "//")) {
                m_LineNumber++;
                continue;
            }

            line = Utils::Trim(line);

            // Section
            if (line.front() == '[')
            {
                if (line.size() < 2 || line.back() != ']') {
                    co_yield ParseError{m_LineNumber, "Malformed section"};
                    co_return;
                }

                std::string_view inside = Utils::Trim(line.substr(1, line.size() - 2));

                size_t sectionPos = 0;

                // Section name
                while (sectionPos < inside.size() && !std::isspace(static_cast<unsigned char>(inside[sectionPos]))) {
                    sectionPos++;
                }

                const std::string_view sectionName = inside.substr(0, sectionPos);
                co_yield SectionEvent{ sectionName };

                // Parse inline fields
                while (sectionPos < inside.size()) {
                    while (sectionPos < inside.size() && std::isspace(static_cast<unsigned char>(inside[sectionPos]))) {
                        sectionPos++;
                    }

                    if (sectionPos >= inside.size()) {
                        break;
                    }

                    // Key
                    const size_t keyStart = sectionPos;
                    while (sectionPos < inside.size()
                        && inside[sectionPos] != '='
                        && !std::isspace(static_cast<unsigned char>(inside[sectionPos]))
                    ) {
                        sectionPos++;
                    }

                    if (sectionPos >= inside.size() || inside[sectionPos] != '=') {
                        co_yield ParseError{m_LineNumber, "Expected '=' in section field"};
                        co_return;
                    }

                    std::string_view key = inside.substr(keyStart, sectionPos - keyStart);
                    sectionPos++; // Skip '='

                    // Value
                    auto valueResult = ParseValueToken(inside, sectionPos, m_LineNumber);

                    if (std::holds_alternative<ParseError>(valueResult)) {
                        co_yield std::get<ParseError>(valueResult);
                        co_return;
                    }

                    std::string_view value;
                    if (std::holds_alternative<std::string_view>(valueResult)) {
                        value = std::get<std::string_view>(valueResult);
                    } else if (std::holds_alternative<std::string>(valueResult)) {
                        value = std::get<std::string>(valueResult);
                    }

                    co_yield FieldEvent{
                        key,
                        value,
                        Utils::GetPathID(key)
                    };
                }

                m_LineNumber++;
                continue;
            }

            // Component
            if (line.front() == '@') {
                co_yield ComponentEvent{ Utils::Trim(line.substr(1)) };
                m_LineNumber++;
                continue;
            }

            // Field
            const size_t eq = line.find('=');
            if (eq == std::string_view::npos) {
                co_yield ParseError{m_LineNumber, "Expected '=' in field"};
                co_return;
            }

            auto key   = Utils::Trim(line.substr(0, eq));
            auto value = Utils::Trim(line.substr(eq + 1));

            co_yield FieldEvent{
                key,
                value,
                Utils::GetPathID(key)
            };

            m_LineNumber++;
        }
    }
}
