#pragma once

#include "AssetManagement/EditorAssetManager.h"
#include "Quelos/Core/Base.h"

namespace QuelosEditor {
    using namespace Quelos;

    class ProjectSerializer {
    public:
        ProjectSerializer() = default;
        explicit ProjectSerializer(const OsPath& projectPath);

        ~ProjectSerializer();

        void Serialize() const;
    private:
        SharedPtr<EditorAssetManager> m_AssetManager;
    };
}
