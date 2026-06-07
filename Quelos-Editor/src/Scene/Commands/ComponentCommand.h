#pragma once

#include "Quelos/Scenes/ComponentRegistery.h"
#include "Quelos/Scenes/Scene.h"

namespace Quelos {
    struct ComponentSnapshot {
        Vec<byte> Data;

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

            ComponentRegistry& registry = scene->GetComponentRegistry();
            if (const auto* componentInfo = registry.GetSerializableComponentInfo(componentId)) {
                entity.Add(componentInfo->RuntimeID);

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
                componentInfo->SerializeBinaryReadFunc(archive, entity.GetMut(componentInfo->RuntimeID));
            }
        }

        static ComponentSnapshot Create(const Ref<Scene>& scene, const Entity entity, const ComponentID componentId) {
            ComponentSnapshot snapshot;

            if (entity.IsValid() && componentId) {
                ComponentRegistry& registry = scene->GetComponentRegistry();
                if (const auto* componentInfo = registry.GetSerializableComponentInfo(componentId)) {
                    Serialization::BinaryWriter writer(snapshot.Data);

                    writer.Write(static_cast<uint64_t>(componentId));

                    Serialization::BinaryWriteArchive archive(writer);
                    componentInfo->SerializeBinaryWriteFunc(archive, entity.GetMut(componentInfo->RuntimeID));
                }
            }

            return snapshot;
        }

        template <typename TComponent>
        static ComponentSnapshot Create(const Ref<Scene>& scene, TComponent& component) {
            ComponentSnapshot snapshot;

            if (const ComponentID componentId = ComponentRegistry::GetComponentID<TComponent>()) {
                ComponentRegistry& registry = scene->GetComponentRegistry();
                if (const auto* componentInfo = registry.GetSerializableComponentInfo(componentId)) {
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
                ComponentRegistry& registry = Scene->GetComponentRegistry();

                if (const auto* componentInfo = registry.GetSerializableComponentInfo(ComponentId)) {
                    actor.Add(componentInfo->RuntimeID);
                }
            }
        }

        void Revert() const {
            if (const Actor actor = Scene->GetActor(EntityId); actor.IsValid()) {
                ComponentRegistry& registry = Scene->GetComponentRegistry();

                if (const auto* componentInfo = registry.GetSerializableComponentInfo(ComponentId)) {
                    actor.Remove(componentInfo->RuntimeID);
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
                ComponentRegistry& registry = Scene->GetComponentRegistry();

                if (const auto* componentInfo = registry.GetSerializableComponentInfo(Snapshot.GetComponentID())) {
                    actor.Remove(componentInfo->RuntimeID);
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
