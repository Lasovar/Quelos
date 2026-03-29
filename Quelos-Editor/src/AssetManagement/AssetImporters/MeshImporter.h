#pragma once
#include "Quelos/AssetManager/Asset.h"
#include "Quelos/AssetManager/AssetMetadata.h"
#include "Quelos/AssetManager/Assets/Mesh.h"

namespace QuelosEditor {
    using namespace Quelos;

    namespace MeshImporter {
        Ref<Mesh> ImportMesh(AssetHandle assetHandle, const AssetMetadata& metadata);
    }
}
