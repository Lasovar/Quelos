#pragma once

namespace Quelos::Utility {
        /// @param relativeFilePath file path relative to the Project path (requires project to be loaded)
        /// @return a Buffer containing the file contents
        Buffer ReadFile(const std::filesystem::path& relativeFilePath);
}
