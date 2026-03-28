#pragma once

#include "Quelos/AssetManager/EditorAssetManager.h"
#include "Quelos/Core/Base.h"

namespace Quelos {
    class ProjectSerializer {
    public:
        ProjectSerializer() = default;
        explicit ProjectSerializer(const Path& projectPath);

        ~ProjectSerializer();

        void Serialize() const;
    private:
        Ref<EditorAssetManager> m_AssetManager;
    };
}
