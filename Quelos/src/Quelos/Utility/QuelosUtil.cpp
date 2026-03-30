#include "qspch.h"
#include "QuelosUtil.h"

#include "Quelos/Core/Application.h"

namespace Quelos::Utility {
    Buffer ReadFile(const std::filesystem::path& relativeFilePath) {
        const Path filePath = Project::GetProjectPath() / relativeFilePath;
        QS_INFO("{}", filePath.string());

        std::ifstream file(filePath, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            QS_CORE_ERROR_TAG("Utility::ReadFile", "Couldn't open file '{}'", filePath.string());
            return {};
        }

        std::streamsize size = file.tellg();
        if (size <= 0) {
            return {};
        }

        Buffer buffer = Buffer::Allocate(size);

        file.seekg(0, std::ios::beg);
        file.read(reinterpret_cast<char*>(buffer.GetData()), size);

        if (!file) {
            QS_CORE_ERROR_TAG("Utility::ReadFile", "Failed to read file '{}'", filePath.string());
            return {};
        }

        return buffer;
    }
}
