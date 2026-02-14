#pragma once

namespace Quelos::Serialization {
    enum class TokenType {
        Identifier,
        String,

        Equals,
        Comma,
        Dot,

        LParen, RParen,
        LCurly, RCurly,
        LSection, RSection, // [ ]

        At, // @

        EndOfLine,
        EndOfFile,
        Error
    };

    inline std::string TokenTypeToString(const TokenType type) {
        switch (type) {
            case TokenType::Identifier: return std::string{"Identifier"};
            case TokenType::String: return std::string{"String"};
            case TokenType::Equals: return std::string{"Equals"};
            case TokenType::Comma: return std::string{"Comma"};
            case TokenType::Dot: return std::string{"Dot"};
            case TokenType::LParen: return std::string{"LParen"};
            case TokenType::RParen: return std::string{"RParen"};
            case TokenType::LCurly: return std::string{"LCurly"};
            case TokenType::LSection: return std::string{"LSection"};
            case TokenType::RCurly: return std::string{"RCurly"};
            case TokenType::RSection: return std::string{"RSection"};
            case TokenType::At: return std::string{"At"};
            case TokenType::EndOfLine: return std::string{"EndOfLine"};
            case TokenType::EndOfFile: return std::string{"EndOfFile"};
            case TokenType::Error: return std::string{"Error"};
            default: return std::string{"Unknown"};
        }
    }

    struct Token {
        TokenType Type{};
        std::string_view Text{};
        size_t Line{};
    };

    class Lexer {
    public:
        explicit Lexer(const std::string_view input)
            : m_Input(input) { }

        Token Next();
        Token Peek();

    private:
        void SkipWhitespace();
        Token ReadIdentifier();
        Token ReadString();

        std::string_view OwnString(std::string&& string);

    private:
        bool m_HasPeeked = false;
        Token m_Peeked{};
        std::string_view m_Input;
        size_t m_Line = 1;

        std::vector<std::string> m_StringPool;
    };
}
