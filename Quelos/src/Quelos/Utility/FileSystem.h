// ReSharper disable CppDFALocalValueEscapesFunction
#pragma once

#include <string_view>
#include <ranges>

#include "Generator.h"

namespace Quelos {
    namespace FS {
        inline std::string_view Filename(const std::string_view path) {
            const size_t pos = path.find_last_of("/\\");
            if (pos == std::string_view::npos) {
                // ReSharper disable once CppDFALocalValueEscapesFunction
                return path;
            }

            return { path.data() + pos + 1, path.size() - (pos + 1) };
        }

        inline std::string_view Extension(const std::string_view path) {
            const size_t pos = path.find_last_of('.');
            if (pos == std::string_view::npos) {
                // ReSharper disable once CppDFALocalValueEscapesFunction
                return { path.data() + path.size(), 0 };
            }

            return { path.data() + pos, path.size() - pos };
        }

        inline std::string_view Stem(const std::string_view path) {
            const size_t slash = path.find_last_of("/\\");
            const size_t filenameStart = slash == std::string_view::npos ? 0 : slash + 1;

            const size_t dot = path.find_last_of('.');

            if (dot == std::string_view::npos || dot <= filenameStart) {
                return path.substr(filenameStart);
            }

            return path.substr(filenameStart, dot - filenameStart);
        }

        constexpr std::string_view Parent(const std::string_view path) {
            if (path.empty()) {
                return {};
            }

            size_t end = path.size();
            while (end > 0 && (path[end - 1] == '/' || path[end - 1] == '\\')) {
                --end;
            }

            if (end == 0) {
                return path;
            }

            const size_t slash = path.find_last_of("/\\", end - 1);

            if (slash == std::string_view::npos) {
                return path;
            }

            // Root
            if (slash == 0) {
                return path.substr(0, 1);
            }

#ifdef QS_PLATFORM_WINDOWS
            // Handle "C:\"
            if (slash == 2 && path[1] == ':')
                return path.substr(0, 3);
#endif

            return path.substr(0, slash);
        }

        inline Generator<std::string_view> Parts(const std::string_view path) {
            size_t start = 0;
            for (size_t i = 0; i <= path.size(); i++) {
                if (i != path.size() && path[i] != '/') {
                    continue;
                }

                std::string_view part(path.data() + start, i - start);

                co_yield part;

                start = i + 1;
            }
        }

        // "Assets/texture.png.qmeta" -> "Assets/texture.png"
        constexpr std::string_view RemoveExtension(const std::string_view path) {
            const size_t nameStart = path.find_last_of("/\\") + 1;
            const size_t dot = path.rfind('.');
            if (dot == std::string_view::npos || dot <= nameStart) {
                return path; // no extension, or a dotfile like ".gitignore"
            }

            return path.substr(0, dot);
        }

        /// Resolves "." and ".." lexically into `out`.
        /// Returns false if a ".." climbs above the start of the path.
        inline bool NormalizedParts(const std::string_view path, Vec<std::string_view>& out) {
            out.clear();
            for (const std::string_view part : Parts(path)) {
                if (part.empty() || part == ".") {
                    continue; // "a//b" and "a/./b" are both "a/b"
                }

                if (part == "..") {
                    if (out.empty()) {
                        return false;  // escapes the root it's relative to
                    }

                    out.pop_back();
                    continue;
                }

                out.push_back(part);
            }

            return true;
        }

        /// True if `path` is inside `dir` (both relative to the same root).
        /// With allowEqual, the directory itself also counts.
        inline bool IsInsideDirectory(
            const std::string_view path,
            const std::string_view dir,
            const bool allowEqual = false
        ) {
            thread_local Vec<std::string_view> pathParts(Allocator::Persistent);
            thread_local Vec<std::string_view> dirParts(Allocator::Persistent);

            pathParts.clear();
            dirParts.clear();

            if (!NormalizedParts(path, pathParts) || !NormalizedParts(dir, dirParts)) {
                return false;
            }

            const uint32_t minSize = dirParts.size() + (allowEqual ? 0 : 1);
            if (pathParts.size() < minSize) {
                return false;
            }

            return std::ranges::equal(dirParts, pathParts | std::views::take(dirParts.size()));
        }

        struct FileStamp {
            uint64_t Size = 0;
            uint64_t TimeStamp = 0;
        };

        inline FileStamp GetFileStamp(const OsPath& path) {
            using namespace std::chrono;

            FileStamp stamp;

            if (!std::filesystem::exists(path)) {
                return stamp;
            }

            stamp.Size = std::filesystem::file_size(path);
            stamp.TimeStamp = time_point_cast<seconds>(std::filesystem::last_write_time(path)).time_since_epoch().count();

            if (std::filesystem::is_regular_file(path)) {
                stamp.Size = std::filesystem::file_size(path);
            } else {
                stamp.Size = 0;
            }

            return stamp;
        }
    }
}
