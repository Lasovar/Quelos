#pragma once

#include "flecs.h"
#include "SetParentCommand.h"
#include "Quelos/Scenes/Components.h"
#include "Quelos/Scenes/Scene.h"

namespace Quelos {
    struct ReorderChild : SetEntityParent {
        void Apply() const {
            const Actor actor = Scene->GetActor(ActorId).GetInternalID();

            if (NewNextActor) {
                const Actor nextActor = Scene->GetActor(NewNextActor).GetInternalID();
                actor.OrderAfter(nextActor);
            } else {
                actor.OrderFront(Scene->GetActor(NewParentId));
            }
        }

        void Revert() const {
            const Actor actor = Scene->GetActor(ActorId).GetInternalID();

            if (PreviousNextActor) {
                const Actor after = Scene->GetActor(PreviousNextActor);
                actor.OrderAfter(after);
            } else {
                actor.OrderFront(Scene->GetActor(PreviousParentId));
            }
        }

        ReorderChild() = default;
        ReorderChild(
            const EntityID actorId,
            const EntityID parentId,
            const EntityID moveAfterId,
            const Ref<Quelos::Scene>& scene
        ) : SetEntityParent(actorId, parentId, scene), NewNextActor(moveAfterId)
        {
            bool found = false;
            EntityID prevActorId;
            Scene->GetActor(PreviousParentId).GetInternalID().children([&](const flecs::entity entity) {
                if (found) {
                    return;
                }

                const EntityID childId = entity.get<EntityID>();
                if (childId == ActorId) {
                    found = true;
                    PreviousNextActor = prevActorId;
                }

                prevActorId = childId;
            });
        }

        EntityID NewNextActor;
        EntityID PreviousNextActor;
    };
}
