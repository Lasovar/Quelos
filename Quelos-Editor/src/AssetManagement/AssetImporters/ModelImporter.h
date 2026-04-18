#pragma once

#include "Quelos/AssetManager/Asset.h"
#include "Quelos/AssetManager/AssetImporter.h"
#include "Quelos/AssetManager/AssetMetadata.h"
#include "Quelos/AssetManager/Assets/Mesh.h"
#include "Quelos/AssetManager/Assets/Model.h"

namespace QuelosEditor {
    using namespace Quelos;

    struct MeshMetadata {
        std::string Name;
        AssetHandle Handle;
    };

    struct MaterialMetadata {
        std::string Name;
        AssetHandle Handle;
    };

    struct ModelMetadata {
        AssetHandle ModelHandle;
        Vec<MeshMetadata> MeshesMetadata;
        Vec<MaterialMetadata> MaterialsMetadata;
    };

    namespace ModelImporter {
        bool IsAssetSupported(std::string_view path);
        Ref<Model> ImportModel(AssetHandle assetHandle, const AssetMetadata& metadata);
        
        std::optional<AssetHandle> ReadAssetHandle(std::string_view assetPath);
        bool WriteAssetHandle(std::string_view assetPath, const AssetHandle& handle);

        inline AssetImporterConfig GetImporterConfig() {
            return { 
                Model::GetStaticType(),
                ImportModel, 
                IsAssetSupported,
                ReadAssetHandle,
                WriteAssetHandle
            };
        }
    }
}
