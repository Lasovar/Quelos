#pragma once

#include "AssetManagement/EditorAssetImporter.h"
#include "Quelos/AssetManager/AssetRegistryExtensions.h"
#include "Quelos/Renderer/Shader.h"

namespace QuelosEditor {
    using namespace Quelos;

    namespace ShaderImporter {
        bool IsAssetSupported(std::string_view assetPath);

        void RecompileShader(const Ref<Shader>& shader);
        Ref<Shader> Import(AssetHandle handle, const AssetMetadata& metadata);

        AssetHandle ReadAssetHandle(std::string_view assetPath);
        bool WriteAssetHandle(std::string_view assetPath, const AssetHandle& handle);

        bool HandlesAssetType(const AssetType& type, void* userData);

        EditorAssetImporterConfig GetImporterConfig();

        void Initialize();
    }
}
