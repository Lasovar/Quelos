#include "qspch.h"
#include "ModelAssetRegistryExtension.h"
#include "Quelos/AssetManager/AssetImporter.h"
#include "Quelos/AssetManager/AssetRegistryExtensions.h"
#include "Quelos/Project/Project.h"

namespace QuelosEditor {
    using namespace Quelos;

    static ModelAssetRegistryExtension s_ModelExtension;

    void ModelAssetRegistryExtension::Register() {
        AssetRegistryExtensions::RegisterExtension(
            AssetRegistryExtension<ModelAssetRegistryExtension>::Create(s_ModelExtension)
        );
    }

    Vec<AssetMetadata> ModelAssetRegistryExtension::RegisterAdditionalAssets(
        const std::string_view assetPath,
        const AssetMetadata& mainAssetMetadata
    ) {
        if (mainAssetMetadata.Type != Model::GetStaticType()) {
            return {};
        }

        return RegisterModelSubAssets(assetPath, mainAssetMetadata);
    }

    Ref<Asset> ModelAssetRegistryExtension::ResolveSubAsset(
        const AssetHandle& subAssetHandle,
        const AssetMetadata& subAssetMetadata
    ) {
        if (subAssetMetadata.Type == Mesh::GetStaticType()) {
            return ResolveMeshSubAsset(subAssetHandle, subAssetMetadata);
        }

        /*
        if (subAssetMetadata.Type == Material::GetStaticType) {
            return ResolveMaterialSubAsset(subAssetHandle, subAssetMetadata);
        }
        */

        return nullptr;
    }

    Vec<AssetMetadata> ModelAssetRegistryExtension::RegisterModelSubAssets(
        const std::string_view assetPath,
        const AssetMetadata& modelMetadata
    ) {
        Vec<AssetMetadata> subAssets;

        // Check if there's existing metadata file with sub-assets
        OsPath assetFile(assetPath);
        if (assetFile.is_absolute()) {
            assetFile = std::filesystem::relative(assetFile, Project::GetProjectPath());
        }
        OsPath metadataFile = Project::GetProjectPath() / (assetFile.generic_string() + ".quel");

        if (std::filesystem::exists(metadataFile)) {
            Ref<Asset> modelAsset = AssetImporter::ImportAsset(modelMetadata.Handle, modelMetadata);

            if (modelAsset && modelAsset->GetAssetType() == Model::GetStaticType()) {
                const Ref<Model> model = RefAs<Model>(modelAsset);

                for (const auto& mesh : model->GetMeshes()) {
                    AssetMetadata meshMetadata = {
                        mesh->GetAssetHandle(),
                        modelMetadata.FilePath,
                        Mesh::GetStaticType(),
                        modelMetadata.Handle,
                        "meshes/" + mesh->GetName()
                    };
                    
                    subAssets.push_back(meshMetadata);
                }
            }
        }

        return subAssets;
    }

    Ref<Asset> ModelAssetRegistryExtension::ResolveMeshSubAsset(
        const AssetHandle& meshHandle,
        const AssetMetadata& meshMetadata
    ) {
        Ref<Model> model = AssetManager::GetAsset<Model>(meshMetadata.ParentHandle);

        if (!model || model->GetAssetType() != Model::GetStaticType()) {
            QS_CORE_ERROR_TAG(
                "ModelAssetRegistryExtension",
                "Failed to load parent model {} for mesh {}",
                meshMetadata.ParentHandle.ToString(), meshHandle.ToString()
            );

            return nullptr;
        }

        if (meshMetadata.VirtualPath.starts_with("meshes/")) {
            const std::string_view meshName = std::string_view(meshMetadata.VirtualPath).substr(7); // Remove "meshes/" prefix
            
            for (const auto& mesh : model->GetMeshes()) {
                if (mesh->GetName() == meshName && mesh->GetAssetHandle() == meshHandle) {
                    return mesh;
                }
            }
        }

        QS_CORE_ERROR_TAG(
            "ModelAssetRegistryExtension",
            "Could not find mesh {} in model {}",
            meshHandle.ToString(), meshMetadata.ParentHandle.ToString()
        );

        return nullptr;
    }

    Ref<Asset> ModelAssetRegistryExtension::ResolveMaterialSubAsset(
        const AssetHandle& materialHandle,
        const AssetMetadata& materialMetadata
    ) {
        // TODO: Implement material sub-asset resolution when Model supports materials
        // This would load the parent model and extract the material
        return nullptr;
    }
}
