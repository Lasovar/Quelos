#pragma once

#include "Quelos/Scenes/ComponentRegistery.h"
#include "Quelos/Scenes/Scene.h"

namespace Quelos {
    class AddComponentCommand {
    public:
        AddComponentCommand(const ActorID& actorId, Ref<Scene>& scene, const ComponentID& componentId)
            : ActorId(actorId), Scene(scene), ComponentId(componentId) { }

        void Apply() const {
            if (const Actor actor = Scene->GetActor(ActorId); actor.IsValid()) {
                ComponentRegistry& registry = Scene->GetComponentRegistry();

                if (const auto* componentInfo = registry.GetSerializableComponentInfo(ComponentId)) {
                    actor.Add(componentInfo->RuntimeID);
                }
            }
        }

        void Revert() const {
            if (const Actor actor = Scene->GetActor(ActorId); actor.IsValid()) {
                ComponentRegistry& registry = Scene->GetComponentRegistry();

                if (const auto* componentInfo = registry.GetSerializableComponentInfo(ComponentId)) {
                    actor.Remove(componentInfo->RuntimeID);
                }
            }
        }

        ActorID ActorId;
        Ref<Scene>& Scene;

        ComponentID ComponentId;
    };

    class RemoveComponentCommand {
    public:
        RemoveComponentCommand(const ActorID& actorId, Ref<Scene>& scene, const ComponentID& componentId)
            : ActorId(actorId), Scene(scene), ComponentId(componentId) { }

        void Apply() {
            if (const Actor actor = Scene->GetActor(ActorId); actor.IsValid()) {
                ComponentRegistry& registry = Scene->GetComponentRegistry();

                if (const auto* componentInfo = registry.GetSerializableComponentInfo(ComponentId)) {
                    ComponentData.clear();
                    Serialization::BinaryWriter writer(ComponentData);
                    Serialization::BinaryWriteArchive archive(writer);

                    componentInfo->SerializeBinaryWriteFunc(archive, actor.GetMut(componentInfo->RuntimeID));

                    actor.Remove(componentInfo->RuntimeID);
                }
            }
        }

        void Revert() const {
            if (const Actor actor = Scene->GetActor(ActorId); actor.IsValid()) {
                ComponentRegistry& registry = Scene->GetComponentRegistry();

                if (const auto* componentInfo = registry.GetSerializableComponentInfo(ComponentId)) {
                    actor.Add(componentInfo->RuntimeID);

                    Serialization::BinaryReader reader(ComponentData);
                    Serialization::BinaryReadArchive archive(reader);

                    componentInfo->SerializeBinaryReadFunc(archive, actor.GetMut(componentInfo->RuntimeID));
                }
            }
        }

        ActorID ActorId;
        Ref<Scene>& Scene;
        Vec<byte> ComponentData;

        ComponentID ComponentId;
    };
}
