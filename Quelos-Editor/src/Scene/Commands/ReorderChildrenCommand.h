#pragma once

#include "flecs.h"
#include "SetParentCommand.h"
#include "Quelos/Scenes/Components.h"
#include "Quelos/Scenes/Scene.h"

namespace Quelos {
    struct ReorderChild : SetParent {
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
            const ActorID actorId,
            const ActorID parentId,
            const ActorID moveAfterId,
            const Ref<Quelos::Scene>& scene
        ) : SetParent(actorId, parentId, scene), NewNextActor(moveAfterId)
        {
            bool found = false;
            ActorID prevActorId;
            Scene->GetActor(PreviousParentId).GetInternalID().children([&](const flecs::entity entity) {
                if (found) {
                    return;
                }

                const ActorID childId = entity.get<ActorTag>().ID;
                if (childId == ActorId) {
                    found = true;
                    PreviousNextActor = prevActorId;
                }

                prevActorId = childId;
            });
        }

        ActorID NewNextActor;
        ActorID PreviousNextActor;
    };
}
