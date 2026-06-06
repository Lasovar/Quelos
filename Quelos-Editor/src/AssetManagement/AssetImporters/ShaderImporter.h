#pragma once

#include "AssetManagement/EditorAssetImporter.h"
#include "Quelos/AssetManager/AssetRegistryExtensions.h"
#include "Quelos/Renderer/GraphicsShader.h"

namespace QuelosEditor {
    using namespace Quelos;

    namespace ShaderImporter {
        bool IsAssetSupported(std::string_view assetPath);

        bool RecompileShader(GraphicsShader* shader);
        bool Cook(const AssetMetadata& metadata);
        bool Import(void* dataSlot, const AssetMetadata& metadata);
        bool Reimport(void* shader, const AssetMetadata& metadata);

        AssetID ReadAssetHandle(std::string_view assetPath);
        bool WriteAssetHandle(std::string_view assetPath, const AssetID& handle);

        bool HandlesAssetType(const AssetType& type, void* userData);

        EditorAssetImporterConfig GetImporterConfig();

        void Initialize();
    }
}
