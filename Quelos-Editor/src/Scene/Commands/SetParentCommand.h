#pragma once

#include "Quelos/Scenes/Scene.h"

namespace Quelos {
    struct SetParent {
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

        SetParent() = default;
        SetParent(const ActorID actorId, const ActorID parentId, const Ref<Scene>& scene) {
            ActorId = actorId;
            NewParentId = parentId;
            Scene = scene;

            const Actor actor = Scene->GetActor(ActorId);
            if (const Actor previousParent = actor.GetParent(); previousParent.IsValid()) {
                PreviousParentId = previousParent.GetActorID();
            }
        }

        ActorID ActorId{};
        ActorID NewParentId{};
        ActorID PreviousParentId{};

        Ref<Scene> Scene;
    };
}
