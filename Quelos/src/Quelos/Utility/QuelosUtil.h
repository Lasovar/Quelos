#pragma once

namespace Quelos::Utility {
        bool ReadBinaryFile(const std::filesystem::path& relativeFilePath, void* buffer, size_t size);
        std::vector<byte> ReadBinaryFile(const std::filesystem::path& relativeFilePath);
}
