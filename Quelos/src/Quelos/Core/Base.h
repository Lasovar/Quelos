#pragma once

#include <deque>
#include <span>

// can't compile ska without this... MSVC throws an exception that std doesn't contain std::out_of_range without this
#include "Quelos/Utility/FlatMap.h"
#include "Quelos/Utility/OrderedVec.h"
#include "Quelos/Utility/OrderedSet.h"
#include "unordered_dense.h"

#include <filesystem>

#if defined(_WIN32)
    #if defined(QS_LIB_SHARED)
        #if defined(QS_LIB_BUILD)
            #define QS_API __declspec(dllexport)
        #else
            #define QS_API __declspec(dllimport)
        #endif
    #else
        #define QS_API
    #endif
#else
    #if defined(QS_LIB_SHARED)
        #define QS_API __attribute__((visibility("default")))
    #else
        #define QS_API
    #endif
#endif

#ifdef IMGUI_API
#undef IMGUI_API
#endif
#define IMGUI_API QS_API
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"

#ifdef QS_ENABLE_PROFILING
#ifndef TRACY_ENABLE
#define TRACY_ENABLE
#endif
#include "tracy/Tracy.hpp"
#define QS_PROFILE_SCOPED() ZoneScoped
#define QS_PROFILE_SCOPED_N(name) ZoneScopedN(name)
#define QS_PROFILE_SCOPED_C(color) ZoneScopedC(color)
#define QS_PROFILE_SCOPED_NC(name, color) ZoneScopedNC(name, color)
#define QS_PROFILE_FRAME() FrameMark
#define QS_PROFILE_FRAME_NAMED(name) FrameMarkNamed(name)
#define QS_PROFILE_FRAME_START(name) FrameMarkStart(name)
#define QS_PROFILE_FRAME_END(name) FrameMarkEnd(name)
#else
#define QS_PROFILE_SCOPED()
#define QS_PROFILE_SCOPED_N(name)
#define QS_PROFILE_SCOPED_C(color)
#define QS_PROFILE_SCOPED_NC(name, color)
#define QS_PROFILE_FRAME()
#define QS_PROFILE_FRAME_NAMED(name)
#define QS_PROFILE_FRAME_START(name)
#define QS_PROFILE_FRAME_END(name)
#endif

#define QS_STRINGIFY_IMPL(x) #x
#define QS_STRINGIFY(x) QS_STRINGIFY_IMPL(x)

namespace Quelos {
    using byte = std::byte;
    consteval int GetBit(const int x) { return 1 << x; }

    template <typename T>
    constexpr std::string_view TypeName() {
#if defined(__clang__) || defined(__GNUC__)
        constexpr std::string_view p = __PRETTY_FUNCTION__;
        constexpr std::string_view key = "T = ";
        const size_t start = p.find(key) + key.size();
        const size_t end = p.find(']', start);
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


    template <typename TKey, typename TValue>
    using HashMap = ankerl::unordered_dense::map<TKey, TValue>;

    template <typename TKey, typename TValue>
    using SegmentedHashMap = ankerl::unordered_dense::segmented_map<TKey, TValue>;

    template <typename TKey, typename TValue>
    using OrderedMap = FlatMap<TKey, TValue>;

    template <typename TValue>
    using HashSet = ankerl::unordered_dense::set<TValue>;

    template <typename TValue>
    using SegmentedHashSet = ankerl::unordered_dense::segmented_set<TValue>;

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

    using Path = std::filesystem::path;
}
