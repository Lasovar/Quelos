#pragma once

#include "Quelos/Core/Base.h"

namespace Quelos {
    class ProjectSerializer {
    public:
        ProjectSerializer() = default;
        explicit ProjectSerializer(const Path& projectPath);
    };
}
