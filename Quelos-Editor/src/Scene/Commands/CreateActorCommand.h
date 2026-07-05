#pragma once

#include "Quelos/Scenes/Entity.h"
#include "Quelos/Scenes/Scene.h"
#include "../../../../Quelos/src/Quelos/Scenes/EntitySnapshot.h"

namespace Quelos {
    struct CreateActor {
    public:
        void Apply() const {
            if (ParentId) {
                Scene->CreateActor(ActorId, "NewActor", ParentId);
            } else {
                Scene->CreateActor(ActorId, "NewActor");
            }
        }

        void Revert() const {
            Scene->DestroyEntity(ActorId);
        }

        CreateActor() = default;
        CreateActor(const EntityID entityId, const Ref<Scene>& scene) : CreateActor(entityId, {}, scene) { }
        CreateActor(const EntityID entityId, const EntityID parentId, const Ref<Scene>& scene) {
            ActorId = entityId;
            ParentId = parentId;
            Scene = scene;
        }

        EntityID ActorId{};
        EntityID ParentId;
        Ref<Scene> Scene;
    };

    struct DestroyActor {
    public:
        void Apply() const {
            Scene->DestroyEntity(ActorId);
        }

        void Revert() const {
            Snapshot.Load(Scene);
        }

        DestroyActor() = default;
        DestroyActor(const EntityID actorId, const Ref<Scene>& scene) {
            ActorId = actorId;
            Scene = scene;
            Snapshot = EntitySnapshot::Create(scene, actorId);
        }

        EntityID ActorId;
        EntitySnapshot Snapshot;
        Ref<Scene> Scene;
    };
}
