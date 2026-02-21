#pragma once
#include "flecs.h"
#include "xxhash.h"

#include "Quelos/Core/GUID.h"
#include "Quelos/Core/Log.h"

#include "Quelos/Serialization/QuelArchive.h"

#include "Components.h"

namespace Quelos {
    using ComponentID = GUID64;

    struct ComponentTypeInfo {
        ecs_id_t RuntimeID{};
        ComponentID Guid{};

        size_t Size{};
        ecs_component_desc_t ComponentDesc{};
    };

    struct SerializableComponentInfo {
        ecs_id_t RuntimeID{};
        ComponentID Guid{};

        // serialization callbacks
        std::function<void(Serialization::BinaryWriteArchive& archive, void* data)> SerializeBinaryWriteFunc = nullptr;
        std::function<void(Serialization::QuelWriteArchive& archive, void* data)> SerializeTextWriteFunc = nullptr;
        std::function<void(Serialization::BinaryReadArchive& archive, void* data)> SerializeBinaryReadFunc = nullptr;
        std::function<void(Serialization::QuelReadArchive& archive, void* data)> SerializeTextReadFunc = nullptr;
    };

    class ComponentRegistry {
    public:
        void RegisterBuiltinTypes(flecs::world& world);

        template <typename T>
        static constexpr std::string_view TypeName() {
#if defined(__clang__) || defined(__GNUC__)
            constexpr std::string_view p = __PRETTY_FUNCTION__;
            constexpr std::string_view key = "T = ";
            constexpr size_t start = p.find(key) + key.size();
            constexpr size_t end = p.find(']', start);
            return p.substr(start, end - start);
#elif defined(_MSC_VER)
            constexpr std::string_view p = __FUNCSIG__;
            constexpr std::string_view key = "type_name<";
            const size_t start = p.find(key) + key.size();
            const size_t end = p.find(">(void)");
            return p.substr(start, end - start);
#else
#   error Unsupported compiler
#endif
        }

        static constexpr ComponentID GetComponentID(const std::string_view& typeName) {
            return ComponentID(XXH3_64bits(typeName.data(), typeName.size()));
        }

        void RegisterType(const ComponentTypeInfo& info) {
            m_ComponentTypes[info.Guid] = info;
        }

        [[nodiscard]] std::unordered_map<ComponentID, ComponentTypeInfo> GetComponents() {
            return m_ComponentTypes;
        }

        [[nodiscard]] std::unordered_map<ComponentID, SerializableComponentInfo>& GetSerializableComponents() {
            return m_SerializableComponents;
        }

        [[nodiscard]] ComponentTypeInfo GetComponentInfo(const ComponentID& componentId) {
            const auto it = m_ComponentTypes.find(componentId);
            if (it != m_ComponentTypes.end()) {
                return it->second;
            }

            return ComponentTypeInfo{};
        }

        [[nodiscard]] SerializableComponentInfo GetSerializableComponentInfo(const ComponentID& componentId) {
            auto it = m_SerializableComponents.find(componentId);
            if (it != m_SerializableComponents.end()) {
                return it->second;
            }

            QS_CORE_ERROR("Can't find component: {}", componentId.ToString());
            return SerializableComponentInfo{};
        }

        void RegisterComponent(
            const flecs::world& world,
            const ComponentID componentId,
            const int32_t size,
            const int32_t alignment
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

            info.RuntimeID = ecs_component_init(world.c_ptr(), &info.ComponentDesc);
            // TODO: figure this shit out
            /*info.SerializeFunc = [](Serialization::BinaryWriter& w, const void* data) {
                w.Write(*static_cast<const T*>(data));
            };

            info.DeserializeFunc = [](Serialization::BinaryReader& r, void* data) {
                //TODO: *static_cast<T*>(data) = r.Read<T>();
            };*/

            RegisterType(info);

            m_ComponentTypes[info.Guid] = info;
        }

        template <typename TComponent>
        void RegisterComponent(
            const flecs::world& world,
            const ComponentID componentId
        ) {
            ComponentTypeInfo info;
            info.Guid = componentId;
            info.Size = sizeof(TComponent);

            info.RuntimeID = world.id<TComponent>();

            RegisterType(info);

            if constexpr (IsSerializable<TComponent>) {
                SerializableComponentInfo serializableInfo;
                serializableInfo.Guid = componentId;
                serializableInfo.RuntimeID = info.RuntimeID;

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

                m_SerializableComponents[serializableInfo.Guid] = serializableInfo;
            }

            m_ComponentTypes[info.Guid] = info;
        }

        template <typename... TComponent>
        void RegisterComponent(flecs::world& world) {
            ([&] {
                RegisterComponent<TComponent>(world, GetComponentID(TypeName<TComponent>()));
            }(), ...);
        }

        template <typename... Component>
        void RegisterComponents(ComponentGroup<Component...>, flecs::world& world) {
            RegisterComponent<Component...>(world);
        }

    private:
        std::unordered_map<ComponentID, ComponentTypeInfo> m_ComponentTypes;
        std::unordered_map<ComponentID, SerializableComponentInfo> m_SerializableComponents;
    };
}
