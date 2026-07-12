#pragma once

#include "API.h"

#include <string>
#include <string_view>

#define QS_STRINGIFY_IMPL(x) #x
#define QS_STRINGIFY(x) QS_STRINGIFY_IMPL(x)

namespace Quelos {
    consteval int GetBit(const int x) { return 1 << x; }

    template <typename T>
    constexpr std::string_view TypeName() {
#if defined(__clang__) || defined(__GNUC__)
        constexpr std::string_view p = __PRETTY_FUNCTION__;
        constexpr std::string_view key = "T = ";
        constexpr size_t start = p.find(key) + key.size();
        constexpr size_t end = p.find(']', start);
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

    template <class... Ts>
    struct Overloaded : Ts... {
        using Ts::operator()...;
    };

    template <class... Ts>
    Overloaded(Ts...) -> Overloaded<Ts...>;
}
