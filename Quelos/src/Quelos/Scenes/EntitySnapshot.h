#pragma once

#include "Quelos/Scenes/Entity.h"

namespace Quelos {
    class Scene;

    struct QS_API EntitySnapshot {
        Vec64<byte> Data{Allocator::Persistent};

        Entity Load(const SharedPtr<Scene>& scene) const; // NOLINT(*-use-nodiscard)

        static EntitySnapshot Create(const SharedPtr<Scene>& scene, EntityID entityId);
        static Entity Load(const SharedPtr<Scene>& scene, const BufferView& entitySnapshot);
    };
}
