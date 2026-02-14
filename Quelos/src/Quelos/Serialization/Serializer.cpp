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

    Generator<ParserEvent> Parser::Parse() {
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
                Token eq = m_Lexer.Next();

                if (eq.Type != TokenType::Equals) {
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

    Generator<ParserEvent> Parser::ParseValue() {
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
}
