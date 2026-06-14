#include "qspch.h"
#include "ComponentRegistery.h"

#include "xxhash.h"

#include "Quelos/Scenes/Components.h"

namespace Quelos {
    static HashMap<ComponentID, ComponentTypeInfo> s_ComponentInfos;
    static HashMap<ComponentID, SerializableComponentInfo> s_SerializableComponentInfos;

    ComponentID ComponentRegistry::GetComponentID(const std::string_view typeName) {
        return ComponentID(XXH3_64bits(typeName.data(), typeName.size()));
    }

    HashMap<ComponentID, ComponentTypeInfo>& ComponentRegistry::GetComponents() {
        return s_ComponentInfos;
    }

    HashMap<ComponentID, SerializableComponentInfo>& ComponentRegistry::GetSerializableComponents() {
        return s_SerializableComponentInfos;
    }

    const ComponentTypeInfo* ComponentRegistry::GetComponentInfo(const ComponentID& componentId) {
        const auto it = s_ComponentInfos.find(componentId);
        if (it != s_ComponentInfos.end()) {
            return &it->second;
        }

        return nullptr;
    }

    const SerializableComponentInfo* ComponentRegistry::GetSerializableComponentInfo(const ComponentID& componentId) {
        const auto it = s_SerializableComponentInfos.find(componentId);
        if (it != s_SerializableComponentInfos.end()) {
            return &it->second;
        }

        return nullptr;
    }

    void ComponentRegistry::RegisterComponent(
        const flecs::world& world, const ComponentID componentId, const int32_t size, const int32_t alignment
    ) {
        ComponentTypeInfo info;
        info.Guid = componentId;
        info.Size = size;
        info.ComponentDesc = ecs_component_desc_t{
            .type = {
                .size = size,
                .alignment = alignment
            }
        };

        //info.RuntimeID = ecs_component_init(world.c_ptr(), &info.ComponentDesc);

        s_ComponentInfos[info.Guid] = info;
        //m_RuntimeIDTypesLookup[info.RuntimeID] = index;

        // TODO: figure this shit out
        /*info.SerializeFunc = [](Serialization::BinaryWriter& w, const void* data) {
                w.Write(*static_cast<const T*>(data));
            };

            info.DeserializeFunc = [](Serialization::BinaryReader& r, void* data) {
                //TODO: *static_cast<T*>(data) = r.Read<T>();
            };*/
    }

    void ComponentRegistry::RegisterBuiltinTypes(flecs::world& world) {
        RegisterComponents(AllComponents {}, world);
    }
}
