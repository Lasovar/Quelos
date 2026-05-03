#pragma once

namespace Quelos::Utility {
    /// @param filePath file path
    /// @param relative whether the passed path is relative to the Project path
    /// @return a Buffer containing the file contents
    QS_API Buffer ReadFile(const OsPath& filePath, bool relative = true);
    QS_API bool WriteFile(const OsPath& filePath, BufferView buffer);
}
