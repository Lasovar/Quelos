#include "qspch.h"
#include "QuelosUtil.h"

#include "Quelos/Core/Application.h"

namespace Quelos::Utility {
    bool ReadBinaryFile(const std::filesystem::path& relativeFilePath, void* buffer, size_t size) {
        const std::string executable = Application::Get().GetApplicationSpecification().Executable;
        const std::filesystem::path exeDir = std::filesystem::canonical(executable).parent_path();
        const std::filesystem::path filePath = exeDir / relativeFilePath;

        std::ifstream file(filePath, std::ios::binary);
        if (!file.is_open()) {
            QS_CORE_ERROR("Couldn't open file '{}'", filePath.string());
            return false;
        }

        const uintmax_t fileSize = std::filesystem::file_size(filePath);

        if (size < fileSize) {
            QS_CORE_ERROR("Buffer size {} is smaller than file size {} for file '{}'", size, fileSize, filePath.string());
            return false;
        }

        file.read(static_cast<char*>(buffer), static_cast<std::streamsize>(fileSize));
        return true;
    }

    std::vector<byte> ReadBinaryFile(const std::filesystem::path& relativeFilePath) {
        const std::string executable = Application::Get().GetApplicationSpecification().Executable;
        const std::filesystem::path exeDir = std::filesystem::canonical(executable).parent_path();
        const std::filesystem::path filePath = exeDir / relativeFilePath;

        std::ifstream file(filePath, std::ios::binary);
        if (!file.is_open()) {
            QS_CORE_ERROR("Couldn't open file '{}'", filePath.string());
            return std::vector<byte>(0);
        }

        const uintmax_t fileSize = std::filesystem::file_size(filePath);
        auto data = std::vector<byte>(fileSize);

        file.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(fileSize));

        return std::move(data);
    }
}
