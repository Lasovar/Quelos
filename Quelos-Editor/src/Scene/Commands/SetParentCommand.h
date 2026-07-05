#pragma once

#include "Quelos/Scenes/Scene.h"

namespace Quelos {
    struct SetEntityParent {
    public:
        void Apply() const {
            const Actor actor = Scene->GetActor(ActorId);

            if (!NewParentId) {
                Scene->SetActorParentToRoot(actor);
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
                Scene->SetActorParentToRoot(actor);
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
