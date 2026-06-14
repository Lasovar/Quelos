//
// Created by lasovar on 6/7/26.
//

#include "SceneSnapshot.h"
#include "EntitySnapshot.h"

namespace Quelos {
    void SceneSnapshot::Load(const Ref<Scene>& scene, const std::string& sceneName, const BufferView data) {
        using namespace Serialization;

        BinaryReader reader(data);

        // Read header
        SceneHeader header = reader.Read<SceneHeader>().value();

        if (header.Magic != SceneHeader().Magic) {
            QS_ERROR_TAG("SceneSerializer", "Invalid scene magic!");
            return;
        }

        scene->SetName(sceneName);

        const flecs::world& world = scene->GetWorld();

        world.defer_begin();

        for (uint64_t i = 0; i < header.EntityCount; i++) {
            const auto entitySnapshotSize = reader.Read<uint32_t>();
            if (!entitySnapshotSize) {
                QS_ERROR_TAG("SceneSerializer", "Invalid entity snapshot size!");
                continue;
            }

            const auto entitySnapshotBlob = reader.ReadBytes(entitySnapshotSize.value());
            if (entitySnapshotBlob.empty()) {
                QS_ERROR_TAG("SceneSerializer", "Failed to read entity snapshot!");
                continue;
            }

            BufferView entitySnapshotView(entitySnapshotBlob.data(), *entitySnapshotSize);
            EntitySnapshot::Load(scene, entitySnapshotView);
        }

        world.defer_end();
    }

    SceneSnapshot SceneSnapshot::Create(const Ref<Scene>& scene) {
        using namespace Serialization;

        SceneSnapshot snapshot;

        snapshot.SceneName = scene->GetName();
        const auto& types = ComponentRegistry::GetSerializableComponents();

        BinaryWriter writer(snapshot.Data);

        const Entity sceneRoot = scene->GetSceneRoot();

        // Header
        SceneHeader header{};
        header.ComponentTypeCount = types.size();
        header.EntityCount = sceneRoot.ChildrenCount();

        writer.Write(header);

        Vec<EntityID> rootActors;
        rootActors.reserve(header.EntityCount);

        sceneRoot.GetInternalID().children([&rootActors](const flecs::entity child) {
            if (auto* actorTag = child.try_get<EntityID>()) {
                rootActors.push_back(*actorTag);
            }
        });

        for (const auto& actorId : rootActors) {
            EntitySnapshot entitySnapshot = EntitySnapshot::Create(scene, actorId);
            writer.Write<uint32_t>(entitySnapshot.Data.size());
            writer.WriteBytes(Span(entitySnapshot.Data));
        }

        return snapshot;
    }
}
