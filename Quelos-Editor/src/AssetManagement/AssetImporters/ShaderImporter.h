#pragma once

#include "AssetManagement/EditorAssetImporter.h"
#include "Quelos/AssetManager/AssetRegistryExtensions.h"
#include "Quelos/Renderer/GraphicsShader.h"

namespace QuelosEditor {
    using namespace Quelos;

    namespace ShaderImporter {
        bool IsAssetSupported(std::string_view assetPath);

        bool Import(ImportContext& importContext);
        bool Load(const LoadContext& loadContext);

        AssetID ReadAssetHandle(std::string_view assetPath);
        bool WriteAssetHandle(std::string_view assetPath, const AssetID& handle);

        bool HandlesAssetType(const AssetType& type, void* userData);

        Vec<AssetDependency> GetDependencies(const AssetMetadata& assetMetadata, std::pmr::memory_resource* memoryResource);

        EditorAssetImporterConfig GetImporterConfig();

        void Initialize();
    }
}
