#include "qspch.h"
#include "QuelLexer.h"

namespace Quelos::Serialization {
    void QuelLexer::SkipWhitespace() {
        while (!m_Input.empty()) {
            char c = m_Input.front();

            if (c == '\n') {
                m_Input.remove_prefix(1);
                m_Line++;
                continue;
            }

            if (!std::isspace(static_cast<unsigned char>(c))) {
                break;
            }

            m_Input.remove_prefix(1);
        }
    }

    Token QuelLexer::ReadIdentifier() {
        size_t len = 0;

        while (len < m_Input.size()) {
            char c = m_Input[len];

            if (std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '.') {
                len++;
            }
            else {
                break;
            }
        }

        const Token token{
            TokenType::Identifier,
            m_Input.substr(0, len),
            m_Line
        };

        m_Input.remove_prefix(len);
        return token;
    }

    Token QuelLexer::ReadString() {
        m_Input.remove_prefix(1); // Skip opening quote

        size_t i = 0;
        bool hasEscape = false;

        while (i < m_Input.size() && m_Input[i] != '"') {
            if (m_Input[i] == '\\' && i + 1 < m_Input.size()) {
                hasEscape = true;
                i += 2;
            }
            else {
                i++;
            }
        }

        if (i >= m_Input.size()) {
            return {TokenType::Error, {}, m_Line};
        }

        std::string_view raw = m_Input.substr(0, i);
        m_Input.remove_prefix(i + 1); // skip closing quote

        if (!hasEscape) {
            return {TokenType::String, raw, m_Line};
        }

        std::string result;
        result.reserve(raw.size());

        for (size_t j = 0; j < raw.size(); ++j) {
            char c = raw[j];

            if (c == '\\' && j + 1 < raw.size()) {
                char escapedChar = raw[++j];

                switch (escapedChar) {
                case '"': result.push_back('"'); break;
                case '\\': result.push_back('\\'); break;
                case 'n': result.push_back('\n'); break;
                case 't': result.push_back('\t'); break;

                default:
                    result.push_back('\\');
                    result.push_back(escapedChar);
                    break;
                }
            }
            else {
                result.push_back(c);
            }
        }

        return {TokenType::String, OwnString(std::move(result)), m_Line};
    }

    std::string_view QuelLexer::OwnString(std::string&& string) {
        m_StringPool.push_back(std::move(string));
        return m_StringPool.back();
    }

    Token QuelLexer::Next() {
        if (m_HasPeeked) {
            m_HasPeeked = false;
            return m_Peeked;
        }

        SkipWhitespace();

        if (m_Input.empty()) {
            return {TokenType::EndOfFile, {}, m_Line};
        }

        char c = m_Input.front();

        if (c == '\n') {
            m_Input.remove_prefix(1);
            m_Line++;
            return {TokenType::EndOfLine, {}, m_Line - 1};
        }

        // single char tokens
        switch (c) {
        case '=': m_Input.remove_prefix(1); return {TokenType::Equals, {}, m_Line};
        case ',': m_Input.remove_prefix(1); return {TokenType::Comma, {}, m_Line};
        case '.': m_Input.remove_prefix(1); return {TokenType::Dot, {}, m_Line};
        case '(': m_Input.remove_prefix(1); return {TokenType::LParen, {}, m_Line};
        case ')': m_Input.remove_prefix(1); return {TokenType::RParen, {}, m_Line};
        case '[': m_Input.remove_prefix(1); return {TokenType::LSection, {}, m_Line};
        case ']': m_Input.remove_prefix(1); return {TokenType::RSection, {}, m_Line};
        case '{': m_Input.remove_prefix(1); return {TokenType::LCurly, {}, m_Line};
        case '}': m_Input.remove_prefix(1); return {TokenType::RCurly, {}, m_Line};
        case '@': m_Input.remove_prefix(1); return {TokenType::At, {}, m_Line};
        case '"': return ReadString();
        default: ;
        }

        // identifier
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '_') {
            return ReadIdentifier();
        }

        // unknown
        m_Input.remove_prefix(1);
        return {TokenType::Error, std::format("Unexpected character: {}", c), m_Line};
    }

    Token QuelLexer::Peek() {
        if (!m_HasPeeked) {
            m_Peeked = Next();
            m_HasPeeked = true;
        }

        return m_Peeked;
    }
}
