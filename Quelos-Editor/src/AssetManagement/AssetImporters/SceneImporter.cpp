#include "SceneImporter.h"

#include "Scene/SceneSerializer.h"

namespace QuelosEditor {
    namespace SceneImporter {
        bool IsAssetSupported(const std::string_view assetPath) {
            const OsPath path = Project::GetProjectPath() / assetPath;
            if (!std::filesystem::is_directory(path)) {
                return false;
            }

            return std::ranges::any_of(
                std::filesystem::directory_iterator(path),
                [](const auto& entry) {
                    return entry.path().extension() == SceneSerializer::SceneFileExtension;
                });
        }

        Ref<Scene> ImportScene(const AssetID assetHandle, const AssetMetadata& metadata) {
            Ref<Scene> scene = CreateRef<Scene>();
            scene->SetAssetID(assetHandle);

            SceneSerializer sceneSerializer(scene, Project::GetProjectPath() / metadata.FilePath);
            sceneSerializer.Deserialize();

            return scene;
        }
    }
}
