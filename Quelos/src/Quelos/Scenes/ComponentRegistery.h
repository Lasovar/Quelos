#pragma once

#include "flecs.h"

#include "Quelos/Core/GUID.h"

#include "Quelos/Serialization/QuelArchive.h"

#include "Components.h"

namespace Quelos {
    using ComponentID = GUID64;
    using RuntimeID = flecs::id_t;

    struct ComponentTypeInfo {
        ComponentID Guid{};

        size_t Size{};
        ecs_component_desc_t ComponentDesc{};
    };

    struct SerializableComponentInfo {
        ComponentID Guid{};
        std::string FullName;
        std::string Name;
        size_t Size = 0;

        // serialization callbacks
        std::function<void(Serialization::BinaryWriteArchive& archive, void* data)> SerializeBinaryWriteFunc = nullptr;
        std::function<void(Serialization::QuelWriteArchive& archive, void* data)> SerializeTextWriteFunc = nullptr;
        std::function<void(Serialization::BinaryReadArchive& archive, void* data)> SerializeBinaryReadFunc = nullptr;
        std::function<void(Serialization::QuelReadArchive& archive, void* data)> SerializeTextReadFunc = nullptr;
    };

    struct QS_API ComponentIDsMap {
        HashMap<ComponentID, RuntimeID> Value;
    };

    namespace ComponentRegistry {
        QS_API void RegisterBuiltinTypes(flecs::world& world);

        QS_API ComponentID GetComponentID(std::string_view typeName);

        template <typename T>
        QS_API ComponentID GetComponentID() {
            const std::string typeName = TypeNameDisplay<T>();
            return GetComponentID(typeName);
        }

        QS_API HashMap<ComponentID, ComponentTypeInfo>& GetComponents();
        QS_API HashMap<ComponentID, SerializableComponentInfo>& GetSerializableComponents();

        QS_API const ComponentTypeInfo* GetComponentInfo(const ComponentID& componentId);
        QS_API const SerializableComponentInfo* GetSerializableComponentInfo(const ComponentID& componentId);

        QS_API void RegisterComponent(const flecs::world& world, ComponentID componentId, int32_t size, int32_t alignment);

        template <typename TComponent>
        void RegisterComponent(
            const flecs::world& world,
            const ComponentID componentId
        ) {
            if (auto it = GetComponents().find(componentId); it != GetComponents().end()) {
                world.component<TComponent>().set(componentId);
                world.ensure<ComponentIDsMap>().Value[componentId] = world.id<TComponent>();
                return;
            }

            ComponentTypeInfo info;
            info.Guid = componentId;
            info.Size = sizeof(TComponent);

            world.component<TComponent>().set(componentId);
            world.ensure<ComponentIDsMap>().Value[componentId] = world.id<TComponent>();

            GetComponents()[info.Guid] = info;

            if constexpr (IsSerializable<TComponent>) {
                SerializableComponentInfo serializableInfo;
                serializableInfo.Guid = componentId;
                serializableInfo.FullName = TypeNameDisplay<TComponent>();
                serializableInfo.Name = TypeNameShort<TComponent>();
                serializableInfo.Size = info.Size;

                serializableInfo.SerializeBinaryWriteFunc = [](Serialization::BinaryWriteArchive& bArchive, void* data) {
                    TComponent::Serialize(bArchive, *static_cast<TComponent*>(data));
                };

                serializableInfo.SerializeBinaryReadFunc = [](Serialization::BinaryReadArchive& bArchive, void* data) {
                    TComponent::Serialize(bArchive, *static_cast<TComponent*>(data));
                };

                serializableInfo.SerializeTextWriteFunc = [](Serialization::QuelWriteArchive& tArchive, void* data) {
                    TComponent::Serialize(tArchive, *static_cast<TComponent*>(data));
                };

                serializableInfo.SerializeTextReadFunc = [](Serialization::QuelReadArchive& tArchive, void* data) {
                    TComponent::Serialize(tArchive, *static_cast<TComponent*>(data));
                };

                GetSerializableComponents()[componentId] = serializableInfo;
            }
        }

        template <typename... TComponent>
        void RegisterComponent(flecs::world& world) {
            ([&] {
                RegisterComponent<TComponent>(world, GetComponentID(TypeNameDisplay<TComponent>()));
            }(), ...);
        }

        template <typename... Component>
        void RegisterComponents(ComponentGroup<Component...>, flecs::world& world) {
            RegisterComponent<Component...>(world);
        }
    }
}
