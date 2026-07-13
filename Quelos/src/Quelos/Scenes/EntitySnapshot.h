#pragma once

#include "Quelos/Scenes/Entity.h"

namespace Quelos {
    class Scene;

    struct QS_API EntitySnapshot {
        Vec64<byte> Data{Allocator::Persistent};

        Entity Load(const Ref<Scene>& scene) const; // NOLINT(*-use-nodiscard)

        static EntitySnapshot Create(const Ref<Scene>& scene, EntityID entityId);
        static Entity Load(const Ref<Scene>& scene, const BufferView& entitySnapshot);
    };
}
