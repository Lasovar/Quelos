#pragma once

#include <cstdint>
#include <string_view>

namespace Quelos {
    namespace Hash {
        inline constexpr uint32_t k_Fnv1a32_Offset = 2166136261u;
        inline constexpr uint32_t k_Fnv1a32_Prime = 16777619u;

        inline constexpr uint64_t k_Fnv1a64_Offset = 14695981039346656037ull;
        inline constexpr uint64_t k_Fnv1a64_Prime = 1099511628211ull;

        constexpr uint32_t Fnv1a32(const std::string_view str) noexcept {
            uint32_t hash = k_Fnv1a32_Offset;

            for (const unsigned char c : str) {
                hash ^= c;
                hash *= k_Fnv1a32_Prime;
            }

            return hash;
        }

        constexpr uint64_t Fnv1a64(const std::string_view str) noexcept {
            uint64_t hash = k_Fnv1a64_Offset;

            for (const unsigned char c : str) {
                hash ^= c;
                hash *= k_Fnv1a64_Prime;
            }

            return hash;
        }

        constexpr uint64_t Fnv1a64(const void* data, const size_t size) noexcept {
            const auto* bytes = static_cast<const unsigned char*>(data);

            uint64_t hash = k_Fnv1a64_Offset;

            for (size_t i = 0; i < size; i++) {
                hash ^= bytes[i];
                hash *= k_Fnv1a64_Prime;
            }

            return hash;
        }

        template <size_t N>
        consteval uint64_t literal64(const char (&str)[N]) noexcept {
            uint64_t hash = k_Fnv1a64_Offset;

            for (size_t i = 0; i < N - 1/* exclude null terminator */; i++) {
                hash ^= static_cast<unsigned char>(str[i]);
                hash *= k_Fnv1a64_Prime;
            }

            return hash;
        }

        consteval uint64_t operator""_fnv1a64(const char* str, const size_t len) noexcept {
            uint64_t hash = k_Fnv1a64_Offset;

            for (size_t i = 0; i < len; i++) {
                hash ^= static_cast<unsigned char>(str[i]);
                hash *= k_Fnv1a64_Prime;
            }

            return hash;
        }
    }
}
