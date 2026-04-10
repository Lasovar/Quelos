#pragma once

#include "Quelos/Scenes/Actor.h"
#include "Quelos/Scenes/Entity.h"

namespace Quelos {
    class Scene;

    struct EntitySnapshot {
        Vec<byte> Data;

        static EntitySnapshot Create(const Ref<Scene>& scene, EntityID entityId);
        static Entity Load(const Ref<Scene>& scene, const EntitySnapshot& snapshot);
    };
}
