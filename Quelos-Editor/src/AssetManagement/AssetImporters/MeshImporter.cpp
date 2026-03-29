#include "qspch.h"
#include "MeshImporter.h"

#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"
#include "Quelos/Project/Project.h"

namespace QuelosEditor {
    using namespace Quelos;

    namespace MeshImporter {
        Ref<Mesh> ImportMesh(const AssetHandle assetHandle, const AssetMetadata& metadata) {
            Assimp::Importer importer;
            const Path absolutePath = std::filesystem::absolute(metadata.FilePath);
            const aiScene* scene = importer.ReadFile(absolutePath.c_str(),
                                                     aiProcess_CalcTangentSpace |
                                                     aiProcess_Triangulate |
                                                     aiProcess_JoinIdenticalVertices |
                                                     aiProcess_SortByPType);

            if (!scene || !scene->mRootNode) {
                QS_CORE_ERROR_TAG(
                    "MeshImporter::ImportMesh",
                    "Failed to import mesh ({},{}): {}",
                    metadata.FilePath.c_str(),
                    assetHandle.ToString(),
                    importer.GetErrorString()
                );

                return nullptr;
            }

            for (int i = 0; i < scene->mNumMeshes; ++i) {
                aiMesh* mesh = scene->mMeshes[i];
                if (!mesh) {
                    continue;
                }


            }
        }
    }
}
