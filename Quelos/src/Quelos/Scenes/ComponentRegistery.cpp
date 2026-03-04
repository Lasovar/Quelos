#include "qspch.h"
#include "ComponentRegistery.h"

#include "xxhash.h"
#include "Quelos/Scenes/Components.h"

namespace Quelos {
    void ComponentRegistry::RegisterBuiltinTypes(flecs::world& world) {
        RegisterComponents(AllComponents {}, world);
    }

    constexpr ComponentID ComponentRegistry::GetComponentID(const std::string_view& typeName) {
        return ComponentID(XXH3_64bits(typeName.data(), typeName.size()));
    }
}
