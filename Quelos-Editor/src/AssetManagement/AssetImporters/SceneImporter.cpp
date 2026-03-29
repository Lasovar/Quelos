#include "SceneImporter.h"

#include "Scene/SceneSerializer.h"

namespace QuelosEditor {
    namespace SceneImporter {
        bool IsAssetSupported(const Path& assetPath) {
            if (!std::filesystem::is_directory(assetPath)) {
                return false;
            }

            for (auto& entry : std::filesystem::directory_iterator(assetPath)) {
                if (entry.path().extension() == SceneSerializer::SceneFileExtension) {
                    return true;
                }
            }

            return false;
        }

        Ref<Scene> ImportScene(const AssetHandle assetHandle, const AssetMetadata& metadata) {
            Ref<Scene> scene = CreateRef<Scene>();
            scene->SetAssetHandle(assetHandle);

            SceneSerializer sceneSerializer(scene, Project::GetProjectPath() / metadata.FilePath);
            sceneSerializer.Deserialize();

            return scene;
        }
    }
}
