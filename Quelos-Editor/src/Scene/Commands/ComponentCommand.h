#pragma once

#include "Quelos/Scenes/ComponentRegistery.h"
#include "Quelos/Scenes/Scene.h"

namespace Quelos {
    struct ComponentSnapshot {
        Vec64<byte> Data{Allocator::Persistent};

        [[nodiscard]] ComponentID GetComponentID() const {
            if (Data.size() < sizeof(ComponentID)) {
                return {};
            }

            ComponentID componentId;
            std::memcpy(&componentId, Data.data(), sizeof(ComponentID));
            return componentId;
        }

        void Set(const Ref<Scene>& scene, const Entity entity) const {
            if (!entity.IsValid() || Data.empty()) {
                return;
            }

            Serialization::BinaryReader componentBlobReader(Data);
            const ComponentID componentId(*componentBlobReader.Read<uint64_t>());

            if (const auto* componentInfo = ComponentRegistry::GetSerializableComponentInfo(componentId)) {
                const RuntimeID runtimeId = scene->GetWorld().get<ComponentIDsMap>().Value.at(componentInfo->Guid);
                entity.Add(runtimeId);

                HashMap<uint64_t, BufferView> fieldsMap;

                while (componentBlobReader.HasRemaining()) {
                    auto fieldHash = componentBlobReader.Read<uint64_t>();
                    auto fieldSize = componentBlobReader.Read<uint64_t>();

                    if (!fieldHash || !fieldSize) {
                        continue;
                    }

                    fieldsMap[fieldHash.value()] = componentBlobReader.ReadBytes(fieldSize.value());
                }

                Serialization::BinaryReadArchive archive(fieldsMap);
                componentInfo->SerializeBinaryReadFunc(archive, entity.GetMut(runtimeId));
            }
        }

        static ComponentSnapshot Create(const Ref<Scene>& scene, const Entity entity, const ComponentID componentId) {
            ComponentSnapshot snapshot;

            if (entity.IsValid() && componentId) {
                if (const auto* componentInfo = ComponentRegistry::GetSerializableComponentInfo(componentId)) {
                    Serialization::BinaryWriter writer(snapshot.Data);

                    writer.Write(static_cast<uint64_t>(componentId));

                    const RuntimeID runtimeId = scene->GetWorld().get<ComponentIDsMap>().Value.at(componentInfo->Guid);
                    Serialization::BinaryWriteArchive archive(writer);
                    componentInfo->SerializeBinaryWriteFunc(archive, entity.GetMut(runtimeId));
                }
            }

            return snapshot;
        }

        template <typename TComponent>
        static ComponentSnapshot Create(const Ref<Scene>& scene, TComponent& component) {
            ComponentSnapshot snapshot;

            if (const ComponentID componentId = ComponentRegistry::GetComponentID<TComponent>()) {
                if (const auto* componentInfo = ComponentRegistry::GetSerializableComponentInfo(componentId)) {
                    Serialization::BinaryWriter writer(snapshot.Data);
                    writer.Write(static_cast<uint64_t>(componentId));

                    Serialization::BinaryWriteArchive archive(writer);

                    componentInfo->SerializeBinaryWriteFunc(archive, &component);
                }
            }

            return snapshot;
        }
    };

    class AddComponentCommand {
    public:
        AddComponentCommand(const EntityID& actorId, Ref<Scene>& scene, const ComponentID& componentId)
            : EntityId(actorId), Scene(scene), ComponentId(componentId) { }

        void Apply() const {
            if (const Actor actor = Scene->GetEntity(EntityId); actor.IsValid()) {
                if (const auto* componentInfo = ComponentRegistry::GetSerializableComponentInfo(ComponentId)) {
                    actor.Add(Scene->GetWorld().get<ComponentIDsMap>().Value.at(componentInfo->Guid));
                }
            }
        }

        void Revert() const {
            if (const Actor actor = Scene->GetActor(EntityId); actor.IsValid()) {
                if (const auto* componentInfo = ComponentRegistry::GetSerializableComponentInfo(ComponentId)) {
                    actor.Remove(Scene->GetWorld().get<ComponentIDsMap>().Value.at(componentInfo->Guid));
                }
            }
        }

        EntityID EntityId;
        Ref<Scene>& Scene;

        ComponentID ComponentId;
    };

    class RemoveComponentCommand {
    public:
        RemoveComponentCommand(const EntityID& actorId, const Ref<Scene>& scene, const ComponentID& componentId)
            : EntityId(actorId), Scene(scene) {
            Snapshot = ComponentSnapshot::Create(Scene, static_cast<Entity>(Scene->GetActor(actorId)), componentId);
        }

        void Apply() const {
            if (const Actor actor = Scene->GetActor(EntityId); actor.IsValid()) {
                if (const auto* componentInfo = ComponentRegistry::GetSerializableComponentInfo(Snapshot.GetComponentID())) {
                    actor.Remove(Scene->GetWorld().get<ComponentIDsMap>().Value.at(componentInfo->Guid));
                }
            }
        }

        void Revert() const {
            Snapshot.Set(Scene, Scene->GetEntity(EntityId));
        }

        EntityID EntityId;
        Ref<Scene> Scene;
        ComponentSnapshot Snapshot;
    };

    struct SetComponentCommand {
        SetComponentCommand(
            const Ref<Scene>& scene,
            const EntityID entityId,
            ComponentSnapshot&& before,
            ComponentSnapshot&& after
        )
            : EntityId(entityId), Scene(scene)
        {
            Before = std::move(before);
            After = std::move(after);
        }

        void Apply() const {
            After.Set(Scene, Scene->GetEntity(EntityId));
        }

        void Revert() const {
            Before.Set(Scene, Scene->GetEntity(EntityId));
        }

        ComponentSnapshot Before;
        ComponentSnapshot After;

        EntityID EntityId;
        Ref<Scene> Scene;
    };
}
