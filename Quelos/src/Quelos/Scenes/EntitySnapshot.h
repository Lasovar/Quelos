#pragma once

#include "Quelos/Scenes/Entity.h"

namespace Quelos {
    class Scene;

    struct EntitySnapshot {
        Vec<byte> Data;

        Entity Load(const Ref<Scene>& scene) const; // NOLINT(*-use-nodiscard)

        static EntitySnapshot Create(const Ref<Scene>& scene, EntityID entityId);
        static Entity Load(const Ref<Scene>& scene, const BufferView& entitySnapshot);
    };
}
