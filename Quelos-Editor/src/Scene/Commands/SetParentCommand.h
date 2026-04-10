#pragma once

#include "Quelos/Scenes/Scene.h"

namespace Quelos {
    struct SetEntityParent {
    public:
        void Apply() const {
            const Actor actor = Scene->GetActor(ActorId);

            if (!NewParentId) {
                actor.RemoveParent();
                return;
            }

            const Actor parent = Scene->GetActor(NewParentId);
            actor.SetParent(parent);
        }

        void Revert() const {
            const Actor actor = Scene->GetActor(ActorId);

            if (PreviousParentId) {
                const Actor parent = Scene->GetActor(PreviousParentId);
                actor.SetParent(parent);
            } else {
                actor.RemoveParent();
            }
        }

        SetEntityParent() = default;
        SetEntityParent(const EntityID actorId, const EntityID parentId, const Ref<Scene>& scene) {
            ActorId = actorId;
            NewParentId = parentId;
            Scene = scene;

            const Actor actor = Scene->GetActor(ActorId);
            if (const Actor previousParent = actor.GetParent(); previousParent.IsValid()) {
                PreviousParentId = previousParent.GetActorID();
            }
        }

        EntityID ActorId{};
        EntityID NewParentId{};
        EntityID PreviousParentId{};

        Ref<Scene> Scene;
    };
}
