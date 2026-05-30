#pragma once

#include <deque>
#include <span>

// can't compile ska without this... MSVC throws an exception that std doesn't contain std::out_of_range without this
#include "Quelos/Utility/FlatMap.h"
#include "Quelos/Utility/SortedVec.h"
#include "Quelos/Utility/SortedSet.h"
#include "Quelos/Utility/SmallVec.h"
#include "Quelos/Utility/Span32.h"
#include "ankerl/unordered_dense.h"

#include <filesystem>

#include "API.h"
#include "Profiling.h"

#define QS_STRINGIFY_IMPL(x) #x
#define QS_STRINGIFY(x) QS_STRINGIFY_IMPL(x)

namespace Quelos {
    using byte = std::byte;
    consteval int GetBit(const int x) { return 1 << x; }

    inline void* Allocate(const size_t size) {
#ifdef QS_ENABLE_PROFILING
        void* pointer = std::malloc(size);
        TracyAlloc(pointer, size);
        return pointer;
#else
        return std::malloc(size);
#endif
    }

    inline void Free(void* pointer) {
#ifdef QS_ENABLE_PROFILING
        TracyFree(pointer);
#endif
        std::free(pointer);
    }

    template <typename TKey, typename TValue>
    using HashMap = ankerl::unordered_dense::map<TKey, TValue>;

    template <typename TKey, typename TValue>
    using SegmentedMap = ankerl::unordered_dense::segmented_map<TKey, TValue>;

    template <typename TKey, typename TValue>
    using SortedMap = FlatMap<TKey, TValue>;

    template <typename TValue>
    using HashSet = ankerl::unordered_dense::set<TValue>;

    template <typename TValue>
    using SegmentedSet = ankerl::unordered_dense::segmented_set<TValue>;

    template <typename T>
    using Deque = std::deque<T>;

    template <typename T>
    using Vec = std::vector<T>;

    template <typename T>
    using Span = std::span<T>;

    template <typename TFirst, typename TSecond>
    using Pair = std::pair<TFirst, TSecond>;

    using BufferView = Span<const byte>;
    using MutBufferView = Span<byte>;

    using OsPath = std::filesystem::path;

    template <typename T>
    using Option = std::optional<T>;
    inline constexpr std::nullopt_t None { std::nullopt_t::_Construct::_Token };

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
