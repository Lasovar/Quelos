#include "qspch.h"
#include "QuelosUtil.h"

#include "Quelos/Core/Application.h"

namespace Quelos::Utility {
    Buffer ReadFile(const std::filesystem::path& filePath, bool relative) {
        OsPath absolutePath = filePath;
        if (relative) {
            absolutePath = Project::GetProjectPath() / filePath;
        }

        std::ifstream file(absolutePath, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            QS_CORE_ERROR_TAG("Utility::ReadFile", "Couldn't open file '{}'", absolutePath.string());
            return {};
        }

        std::streamsize size = file.tellg();
        if (size <= 0) {
            return {};
        }

        Buffer buffer = Buffer::Allocate(size);

        file.seekg(0, std::ios::beg);
        file.read(reinterpret_cast<char*>(buffer.data()), size);

        if (!file) {
            QS_CORE_ERROR_TAG("Utility::ReadFile", "Failed to read file '{}'", absolutePath.string());
            return {};
        }

        return buffer;
    }

    bool WriteFile(const OsPath& filePath, const BufferView buffer) {
        // Disk write
        std::ofstream file(filePath, std::ios::binary);

        if (!file) {
            return false;
        }

        file.write(
            reinterpret_cast<const char*>(buffer.data()),
            static_cast<std::streamsize>(buffer.size())
        );
        return true;
    }
}
