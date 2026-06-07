// ReSharper disable CppDFALocalValueEscapesFunction
#pragma once

#include <string_view>

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
