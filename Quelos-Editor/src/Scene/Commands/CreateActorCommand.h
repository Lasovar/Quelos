#pragma once

#include "Quelos/Scenes/Entity.h"
#include "Quelos/Scenes/Scene.h"
#include "Scene/EntitySnapshot.h"

namespace Quelos {
    struct CreateActor {
    public:
        void Apply() const {
            Scene->CreateActor(ActorId, "NewActor");
        }

        void Revert() const {
            Scene->DestroyEntity(ActorId);
        }

        CreateActor() = default;
        CreateActor(const ActorID guid64, const Ref<Scene>& scene) {
            ActorId = guid64;
            Scene = scene;
        }

        ActorID ActorId{};
        Ref<Scene> Scene;
    };

    struct DestroyActor {
    public:
        void Apply() const {
            Scene->DestroyEntity(ActorId);
        }

        void Revert() const {
            EntitySnapshot::Load(Scene, Snapshot);
        }

        DestroyActor() = default;
        DestroyActor(const ActorID actorId, const Ref<Scene>& scene) {
            Snapshot = EntitySnapshot::Create(scene, actorId);
            Scene = scene;
        }

        ActorID ActorId;
        EntitySnapshot Snapshot;
        Ref<Scene> Scene;
    };
}
