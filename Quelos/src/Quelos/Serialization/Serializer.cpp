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

        static void RemoveTrailingComma(std::string& string) {
            size_t i = string.size();

            while (i > 0 && std::isspace(static_cast<unsigned char>(string[i - 1]))) {
                i--;
            }

            if (i > 0 && string[i - 1] == ',') {
                string.erase(i - 1, 1);
            }
        }
    }

    Generator<ParserEvent> QuelReader::Parse() {
        Token token = m_Lexer.Next();

        while (token.Type != TokenType::EndOfFile) {
            // Section
            if (token.Type == TokenType::LSection) {
                Token name = m_Lexer.Next();

                if (name.Type != TokenType::Identifier) {
                    co_yield ParseError{name.Line, "Expected section name"};
                    co_return;
                }

                co_yield SectionEvent{name.Text};

                // Inline fields
                while (true) {
                    Token key = m_Lexer.Next();

                    if (key.Type == TokenType::RCurly) {
                        key = m_Lexer.Next();
                    }

                    if (key.Type == TokenType::RSection) {
                        break;
                    }

                    if (key.Type != TokenType::Identifier) {
                        co_yield ParseError{key.Line, std::format("Expected key in section, found {}", TokenTypeToString(key.Type))};
                        co_return;
                    }

                    if (m_Lexer.Next().Type != TokenType::Equals) {
                        co_yield ParseError{key.Line, "Expected '=' in section"};
                        co_return;
                    }

                    co_yield FieldEvent {
                        key.Text,
                        Utils::GetPathID(key.Text)
                    };

                    for (auto&& parseEvent : ParseValue()) {
                        co_yield parseEvent;
                    }

                    if (m_Lexer.Peek().Type == TokenType::RSection) {
                        m_Lexer.Next();
                        break;
                    }
                }
            }
            // Component
            else if (token.Type == TokenType::At) {
                Token name = m_Lexer.Next();

                if (name.Type != TokenType::Identifier) {
                    co_yield ParseError{name.Line, "Expected component name"};
                    co_return;
                }

                co_yield ComponentEvent { name.Text };
            }
            // Field
            else if (token.Type == TokenType::Identifier) {
                if (m_Lexer.Next().Type != TokenType::Equals) {
                    co_yield ParseError{token.Line, "Expected '=' after key"};
                    co_return;
                }

                co_yield FieldEvent { token.Text, Utils::GetPathID(token.Text) };

                for (auto&& parserEvent : ParseValue()) {
                    co_yield parserEvent;
                }
            }

            token = m_Lexer.Next();
        }
    }

    Generator<ParserEvent> QuelReader::ParseValue() {
        Token token = m_Lexer.Next();

        // Tuple
        if (token.Type == TokenType::LParen) {
            co_yield TupleBeginEvent{};

            while (true) {
                for (auto&& parseEvent : ParseValue()) {
                    co_yield parseEvent;
                }

                Token next = m_Lexer.Next();

                if (next.Type == TokenType::RParen) {
                    break;
                }

                if (next.Type != TokenType::Comma) {
                    co_yield ParseError{next.Line, "Expected ',' or ')'"};
                    co_return;
                }
            }

            co_yield TupleEndEvent{};
            co_return;
        }

        // Array
        if (token.Type == TokenType::LCurly) {
            co_yield ArrayBeginEvent{};

            // Empty array
            if (m_Lexer.Peek().Type == TokenType::RCurly) {
                m_Lexer.Next(); // consume }
                co_yield ArrayEndEvent{};
                co_return;
            }

            while (true) {
                for (auto&& parserEvent : ParseValue()) {
                    co_yield parserEvent;
                }

                const Token peak = m_Lexer.Peek();

                if (peak.Type == TokenType::RCurly) {
                    break;
                }

                // Optional comma
                if (peak.Type == TokenType::Comma) {
                    m_Lexer.Next(); // consume
                }

                if (m_Lexer.Peek().Type == TokenType::RCurly) {
                    break;
                }
            }

            co_yield ArrayEndEvent{};
            co_return;
        }

        // Scalar
        if (token.Type == TokenType::Identifier || token.Type == TokenType::String) {
            co_yield ValueEvent { token.Text };
            co_return;
        }

        co_yield ParseError{token.Line, std::format("Invalid value: {}", token.Text)};
    }

    void StringQuelWriter::WriteEscaped(const std::string_view text) const {
        bool needsQuotes = false;

        for (char c : text) {
            if (std::isspace(static_cast<unsigned char>(c)) || c == '"' || c == '\\') {
                needsQuotes = true;
                break;
            }
        }

        if (!needsQuotes) {
            m_Out += text;
            return;
        }

        m_Out += '"';

        for (const char c : text) {
            switch (c)
            {
            case '"':  m_Out += "\\\""; break;
            case '\\': m_Out += "\\\\"; break;
            case '\n': m_Out += "\\n";  break;
            case '\t': m_Out += "\\t";  break;
            default:   m_Out += c;      break;
            }
        }

        m_Out += '"';
    }

    void StringQuelWriter::WriteIndent() {
        if (m_Formatting == QuelFormatting::None) {
            return;
        }

        m_Out.append(m_Indent * 2, ' ');
        m_AtLineStart = false;
    }

    void StringQuelWriter::NewLine() {
        if (m_Formatting == QuelFormatting::None) {
            if (!m_InArray) {
                m_Out.push_back(' ');
            }

            return;
        }

        m_Out.push_back('\n');
        m_AtLineStart = true;
    }

    void StringQuelWriter::CloseSectionHeader() {
        if (m_InSectionHeader) {
            m_Out.push_back(']');
            NewLine();
            m_InSectionHeader = false;
        }
    }

    template<class... Ts>
    struct Overloaded : Ts... {
        using Ts::operator()...;
    };

    template <class... Ts>
    Overloaded(Ts...) -> Overloaded<Ts...>;

    void StringQuelWriter::Write(const ParserEvent& parserEvent) {
            std::visit(Overloaded {
            [&](const SectionEvent& e) {
                m_Out.push_back('[');
                m_Out.append(e.Name);

                m_InSectionHeader = true;
            },
            [&](const ComponentEvent& e) {
                CloseSectionHeader();

                m_Out.push_back('@');
                m_Out.append(e.Name);
                NewLine();
            },
            [&](const FieldEvent& e) {
                if (m_InSectionHeader) {
                    m_Out.push_back(' ');
                }

                m_Out.append(e.Path);

                if (m_Formatting == QuelFormatting::None || m_InSectionHeader) {
                    m_Out.push_back('=');
                } else {
                    m_Out.append(" = ");
                }
            },
            [&](const ValueEvent& e) {
                if (!m_InSectionHeader && m_InArray && !m_InTuple) {
                    WriteIndent();
                }

                WriteEscaped(e.Text);

                if (m_InArray || m_InTuple) {
                    m_Out.push_back(',');
                }

                if (!m_InSectionHeader && !m_InTuple) {
                    NewLine();
                }
            },
            [&](const TupleBeginEvent&) {
                if (m_InArray && !m_InSectionHeader) {
                    WriteIndent();
                }

                m_Out.push_back('(');
                m_InTuple = true;
            },
            [&](const TupleEndEvent&) {
                m_Out.back() = ')';
                if (m_InArray) {
                    m_Out.push_back(',');
                }

                if (!m_InSectionHeader) {
                    NewLine();
                }

                m_InTuple = false;
            },
            [&](const ArrayBeginEvent&) {
                m_Out.push_back('{');
                m_InArray = true;

                if (!m_InSectionHeader) {
                    NewLine();
                }
            },
            [&](const ArrayEndEvent&) {
                Utils::RemoveTrailingComma(m_Out);
                m_Out.push_back('}');
                m_InArray = false;

                if (!m_InSectionHeader) {
                    NewLine();
                }
            },
            [&](const ParseError&) { /* ignored */ }

        }, parserEvent);
    }
}
