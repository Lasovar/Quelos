#pragma once

#include "Quelos/Scenes/Actor.h"
#include "Quelos/Scenes/Entity.h"

namespace Quelos {
    class Scene;

    struct ActorSnapshot {
        Vec<byte> Data;

        static ActorSnapshot Create(const Ref<Scene>& scene, ActorID actorId);
        static Actor Load(const Ref<Scene>& scene, const ActorSnapshot& snapshot);
    };
}
