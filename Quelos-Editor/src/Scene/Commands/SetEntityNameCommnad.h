#pragma once
#include "Quelos/Scenes/Scene.h"

namespace Quelos {
    struct SetEntityName {
        void Apply() const {
            if (const Actor actor = Scene->GetActor(ActorId); actor.IsValid()) {
                actor.SetName(NewName);
            }
        }

        void Revert() const {
            if (const Actor actor = Scene->GetActor(ActorId); actor.IsValid()) {
                actor.SetName(PreviousName);
            }
        }

        SetEntityName(const EntityID actorId, const Ref<Scene>& scene, std::string newName)
            : ActorId(actorId), Scene(scene), NewName(std::move(newName))
        {
            if (const Actor actor = Scene->GetActor(ActorId); actor.IsValid()) {
                PreviousName = actor.GetName();
            }
        }

        EntityID ActorId;
        Ref<Scene> Scene;
        std::string NewName;
        std::string PreviousName;
    };
}
