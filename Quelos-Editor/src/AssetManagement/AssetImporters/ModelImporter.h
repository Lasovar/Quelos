#pragma once

#include "AssetManagement/EditorAssetImporter.h"
#include "Quelos/AssetManager/AssetRegistryExtensions.h"

#include "Quelos/AssetManager/Assets/Mesh.h"
#include "Quelos/AssetManager/Assets/Model.h"
#include "Quelos/Serialization/Serializer.h"

namespace QuelosEditor {
    using namespace Quelos;

    // Unified Model Importer - contains all model-related logic
    namespace ModelImporter {
        // Model metadata structures
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

        // Core importer functions
        bool IsAssetSupported(std::string_view path);
        Ref<Model> ImportModel(AssetHandle assetHandle, const AssetMetadata& metadata);
        
        // Metadata handling for .quel files
        AssetHandle ReadAssetHandle(std::string_view assetPath);
        bool WriteAssetHandle(std::string_view assetPath, const AssetHandle& handle);
        
        // Multi-asset registration (editor-only)
        Vec<AssetMetadata> RegisterModelSubAssets(
            std::string_view assetPath,
            const AssetMetadata& modelMetadata
        );
        
        // Sub-asset resolution (editor-only)
        Ref<Asset> ResolveMeshSubAsset(
            const AssetHandle& meshHandle,
            const AssetMetadata& meshMetadata
        );

        // Asset registry extension functions (editor-only)
        Vec<AssetMetadata> RegisterAdditionalAssets(
            std::string_view assetPath,
            const AssetMetadata& mainAssetMetadata,
            void* userData
        );
        
        Ref<Asset> ResolveSubAsset(
            const AssetHandle& subAssetHandle,
            const AssetMetadata& subAssetMetadata,
            void* userData
        );
        
        bool HandlesAssetType(const AssetType& type, void* userData);

        // Registration helpers
        EditorAssetImporterConfig GetImporterConfig();
        AssetRegistryExtensionFunctions GetRegistryExtensionFunctions();
        
        // Initialize the model importer system
        void Initialize();
    }
}
