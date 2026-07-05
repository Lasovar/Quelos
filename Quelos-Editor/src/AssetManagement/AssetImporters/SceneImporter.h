#pragma once

#include "Quelos/Scenes/Scene.h"
#include "Quelos/AssetManager/Asset.h"
#include "Quelos/AssetManager/AssetImporter.h"
#include "Quelos/AssetManager/AssetMetadata.h"

namespace QuelosEditor {
    using namespace Quelos;

    namespace SceneImporter {
        bool IsAssetSupported(std::string_view assetPath);
        Ref<Scene> ImportScene(AssetID assetHandle, const AssetMetadata& metadata, flecs::world& world);

        inline AssetImporterConfig GetImporterConfig() {
            // TODO: Should not really import a scene as a normal asset
            return { Scene::GetStaticType(), nullptr, IsAssetSupported };
        }
    }
}
