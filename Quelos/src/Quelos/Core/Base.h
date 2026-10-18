#pragma once

#include "API.h"

#include <string>
#include <source_location>
#include <string_view>

#define QS_STRINGIFY_IMPL(x) #x
#define QS_STRINGIFY(x) QS_STRINGIFY_IMPL(x)

/// TypeName needs to be in a different namespace because in gcc
/// if the function TypeName was called from the same namespace
/// gcc will omit that namespace name in the __PRETTY_FUNCTION__ output
/// Making the function in a different namespace will just insure that it works across compilers
/// Quelos namespace has a wrapper for it Quelos::TypeName<T>
namespace QuelosReflect {
    template <typename T>
    constexpr std::string_view TypeName() {
#if defined(__clang__) || defined(__GNUC__)
        constexpr std::string_view p = __PRETTY_FUNCTION__;
        constexpr std::string_view key = "T = ";
        constexpr size_t start = p.find(key) + key.size();
        constexpr size_t end = p.find_first_of(";]", start);
        return p.substr(start, end - start);
#elif defined(_MSC_VER)
        // Maybe try something different? to messy
        constexpr std::string_view p = __FUNCSIG__;
        constexpr std::string_view key = "TypeName<";
        constexpr size_t start = p.find(key) + key.size();
        constexpr size_t end = p.rfind('>');
        std::string_view name = p.substr(start, end - start);
        while (true) {
            const size_t pos = name.find(' ');
            if (pos == std::string_view::npos) {
                break;
            }

            std::string_view prefix = name.substr(0, pos);

            if (prefix == "class" || prefix == "struct" || prefix == "enum" || prefix == "union") {
                name.remove_prefix(pos + 1);
            }
            else {
                break;
            }
        }

        return name;
#else
#   error Unsupported compiler
#endif
    }
}

namespace Quelos {
    consteval int GetBit(const int x) { return 1 << x; }

    template <typename T>
    constexpr std::string_view TypeName() {
        return QuelosReflect::TypeName<T>();
    }

    template<typename T>
    constexpr std::string TypeNameDisplay() {
        constexpr std::string_view name = TypeName<T>();

        std::string result;
        result.reserve(name.size());

        for (size_t i = 0; i < name.size(); i++) {
            if (i + 1 < name.size() && name[i] == ':' && name[i + 1] == ':') {
                result.push_back('.');
                ++i;
            }
            else {
                result.push_back(name[i]);
            }
        }

        return result;
    }

    template <typename T>
    constexpr std::string_view TypeNameShort() {
        constexpr std::string_view name = TypeName<T>();
        const size_t pos = name.rfind("::");
        if (pos == std::string_view::npos) {
            return name;
        }

        return name.substr(pos + 2);
    }

    namespace Detail {
        // Index of the '(' that opens the parameter list, skipping template arguments
        // in the return type/name and the "()" in "operator()".
        constexpr std::size_t FindParameterList(const std::string_view s) {
            int angle = 0;
            for (std::size_t i = 0; i < s.size(); ++i) {
                if (s.substr(i).starts_with("operator")) {
                    // operator<, operator(), operator-> ...
                    i += 8;
                    while (i < s.size() && s[i] != '(') ++i; // skip the operator symbol
                    if (s.substr(i).starts_with("()(")) i += 2; // operator()
                    return i;
                }

                const char c = s[i];
                if (c == '<') {
                    ++angle;
                }
                else if (c == '>') {
                    --angle;
                }
                else if (c == '(' && angle == 0) {
                    return i;
                }
            }

            return std::string_view::npos;
        }

        // Start of the qualified name: just after the last top-level space before `end`
        // (skips the return type and calling conventions like __cdecl).
        constexpr std::size_t FindNameStart(const std::string_view s, const std::size_t end) {
            int angle = 0, paren = 0;
            for (std::size_t i = end; i-- > 0;) {
                const char c = s[i];
                if (c == '>')       { ++angle; }
                else if (c == '<')  { --angle; }
                else if (c == ')')  { ++paren; }
                else if (c == '(')  { --paren; }
                else if (c == ' ' && angle == 0 && paren == 0) { return i + 1; }
            }

            return 0;
        }

        // Keep the last `count` "::"-separated segments (0 = keep all).
        constexpr std::string_view LastSegments(const std::string_view name, std::size_t count) {
            if (count == 0) {
                return name;
            }

            int angle = 0;
            for (std::size_t i = name.size(); i-- > 1;) {
                const char c = name[i];
                if (c == '>')       { ++angle; }
                else if (c == '<')  { --angle; }
                else if (angle == 0 && c == ':' && name[i - 1] == ':' && --count == 0) {
                    return name.substr(i + 1);
                }
            }

            return name;
        }

        constexpr std::size_t FindMatchingParen(const std::string_view s, const std::size_t open) {
            int depth = 0;
            for (std::size_t i = open; i < s.size(); ++i) {
                if (s[i] == '(') { ++depth; }
                else if (s[i] == ')' && --depth == 0) { return i; }
            }

            return s.size() - 1;
        }

        // segments: how many trailing scopes to keep (2 = Class::Function, 0 = fully qualified)
        constexpr std::string_view FunctionTag(
            const std::string_view signature,
            const std::size_t segments = 2,
            const bool withArguments = false
        ) {
            const std::size_t params = FindParameterList(signature);
            if (params == std::string_view::npos) {
                return signature;
            }

            // For operators, scan from the "operator" keyword so symbols like '<' aren't
            // mistaken for template brackets.
            std::size_t scanEnd = signature.substr(0, params).rfind("operator");
            if (scanEnd == std::string_view::npos) {
                scanEnd = params;
            }

            const std::size_t start = FindNameStart(signature, scanEnd);
            const std::string_view qualifier = LastSegments(signature.substr(start, scanEnd - start), segments);
            const std::size_t nameBegin = static_cast<std::size_t>(qualifier.data() - signature.data());
            const std::string_view name = signature.substr(nameBegin, params - nameBegin);
            if (!withArguments) {
                return name;
            }

            const std::size_t close = FindMatchingParen(signature, params);
            return signature.substr(nameBegin, close + 1 - nameBegin);
        }
    }



    template <class... Ts>
    struct Overloaded : Ts... {
        using Ts::operator()...;
    };

    template <class... Ts>
    Overloaded(Ts...) -> Overloaded<Ts...>;
}

#define QS_FUNCTION_TAG      ::Quelos::Detail::FunctionTag(std::source_location::current().function_name())
#define QS_FUNCTION_TAG_ARGS ::Quelos::Detail::FunctionTag(std::source_location::current().function_name(), 2, true)
