#pragma once

#include "Quelos/Scenes/Scene.h"

namespace Quelos {
    struct SetParent {
    public:
        void Apply() const {
            const Entity actor = Scene->GetActor(ActorId);

            if (!NewParentId) {
                actor.RemoveParent();
                return;
            }

            const Entity parent = Scene->GetActor(NewParentId);
            actor.SetParent(parent);
        }

        void Revert() const {
            const Entity actor = Scene->GetActor(ActorId);

            if (PreviousParentId) {
                const Entity parent = Scene->GetActor(PreviousParentId);
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

            const Entity actor = Scene->GetActor(ActorId);
            if (const Entity previousParent = actor.GetParent(); previousParent.IsValid()) {
                PreviousParentId = previousParent.Get<ActorTag>().ID;
            }
        }

        ActorID ActorId{};
        ActorID NewParentId{};
        ActorID PreviousParentId{};

        Ref<Scene> Scene;
    };
}
