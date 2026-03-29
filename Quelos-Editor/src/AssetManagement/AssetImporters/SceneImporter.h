#pragma once

#include "Quelos/Scenes/Scene.h"
#include "Quelos/AssetManager/Asset.h"
#include "Quelos/AssetManager/AssetImporter.h"
#include "Quelos/AssetManager/AssetMetadata.h"

namespace QuelosEditor {
    using namespace Quelos;

    namespace SceneImporter {
        bool IsAssetSupported(const Path& assetPath);
        Ref<Scene> ImportScene(AssetHandle assetHandle, const AssetMetadata& metadata);

        inline AssetImporterConfig GetImporterConfig() {
            return { AssetType::Scene, ImportScene, IsAssetSupported };
        }
    }
}
