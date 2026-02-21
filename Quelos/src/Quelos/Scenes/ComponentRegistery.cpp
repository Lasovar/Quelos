#include "qspch.h"
#include "ComponentRegistery.h"
#include "Quelos/Scenes/Components.h"

namespace Quelos {
    void ComponentRegistry::RegisterBuiltinTypes(flecs::world& world) {
        RegisterComponents(AllComponents {}, world);
    }
}
