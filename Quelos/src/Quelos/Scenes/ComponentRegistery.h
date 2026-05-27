#pragma once

#include "flecs.h"

#include "Quelos/Core/GUID.h"

#include "Quelos/Serialization/QuelArchive.h"

#include "Components.h"

namespace Quelos {
    using ComponentID = GUID64;
    using RuntimeID = flecs::id_t;

    struct ComponentTypeInfo {
        RuntimeID RuntimeID{};
        ComponentID Guid{};

        size_t Size{};
        ecs_component_desc_t ComponentDesc{};
    };

    struct SerializableComponentInfo {
        RuntimeID RuntimeID{};
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

    class QS_API ComponentRegistry {
    public:
        explicit ComponentRegistry(flecs::world& world) {
            RegisterBuiltinTypes(world);
        }

        void RegisterBuiltinTypes(flecs::world& world);

        template <typename T>
        static ComponentID GetComponentID() {
            const std::string typeName = TypeNameDisplay<T>();
            return GetComponentID(typeName);
        }

        static ComponentID GetComponentID(std::string_view typeName);

        [[nodiscard]] Vec<ComponentTypeInfo>& GetComponents() {
            return m_ComponentTypeInfos;
        }

        [[nodiscard]] Vec<SerializableComponentInfo>& GetSerializableComponents() {
            return m_SerializableComponents;
        }

        [[nodiscard]] ComponentTypeInfo* GetComponentInfo(const ComponentID& componentId) {
            const auto it = m_GuidTypesLookup.find(componentId);
            if (it != m_GuidTypesLookup.end()) {
                return &m_ComponentTypeInfos[it->second];
            }

            return nullptr;
        }

        [[nodiscard]] ComponentTypeInfo* GetComponentInfo(const RuntimeID componentId) {
            const auto it = m_RuntimeIDTypesLookup.find(componentId);
            if (it != m_RuntimeIDTypesLookup.end()) {
                return &m_ComponentTypeInfos[it->second];
            }

            return nullptr;
        }

        [[nodiscard]] SerializableComponentInfo* GetSerializableComponentInfo(const ComponentID& componentId) {
            const auto it = m_GuidSerializableLookup.find(componentId);
            if (it != m_GuidSerializableLookup.end()) {
                return &m_SerializableComponents[it->second];
            }

            return nullptr;
        }

        [[nodiscard]] SerializableComponentInfo* GetSerializableComponentInfo(const RuntimeID& componentId) {
            const auto it = m_RuntimeIDSerializableLookup.find(componentId);
            if (it != m_RuntimeIDSerializableLookup.end()) {
                return &m_SerializableComponents[it->second];
            }

            return nullptr;
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

            const size_t index = m_ComponentTypeInfos.size();
            m_ComponentTypeInfos.push_back(info);
            m_GuidTypesLookup[info.Guid] = index;
            m_RuntimeIDTypesLookup[info.RuntimeID] = index;

            // TODO: figure this shit out
            /*info.SerializeFunc = [](Serialization::BinaryWriter& w, const void* data) {
                w.Write(*static_cast<const T*>(data));
            };

            info.DeserializeFunc = [](Serialization::BinaryReader& r, void* data) {
                //TODO: *static_cast<T*>(data) = r.Read<T>();
            };*/
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

            const size_t index = m_ComponentTypeInfos.size();
            m_ComponentTypeInfos.push_back(info);

            m_GuidTypesLookup[info.Guid] = index;
            m_RuntimeIDTypesLookup[info.RuntimeID] = index;

            if constexpr (IsSerializable<TComponent>) {
                SerializableComponentInfo serializableInfo;
                serializableInfo.Guid = componentId;
                serializableInfo.RuntimeID = info.RuntimeID;
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

                size_t serializableIndex = m_SerializableComponents.size();
                m_SerializableComponents.push_back(serializableInfo);
                m_GuidSerializableLookup[componentId] = serializableIndex;
                m_RuntimeIDSerializableLookup[info.RuntimeID] = serializableIndex;
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

    private:
        Vec<ComponentTypeInfo> m_ComponentTypeInfos;

        HashMap<ComponentID, size_t> m_GuidTypesLookup;
        HashMap<RuntimeID, size_t> m_RuntimeIDTypesLookup;

        Vec<SerializableComponentInfo> m_SerializableComponents;

        HashMap<ComponentID, size_t> m_GuidSerializableLookup;
        HashMap<RuntimeID, size_t> m_RuntimeIDSerializableLookup;
    };
}
