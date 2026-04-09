#pragma once

namespace Quelos::Utility {
    /// @param relativeFilePath file path relative to the Project path (requires project to be loaded)
    /// @return a Buffer containing the file contents
    QS_API Buffer ReadFile(const std::filesystem::path& relativeFilePath);
}
