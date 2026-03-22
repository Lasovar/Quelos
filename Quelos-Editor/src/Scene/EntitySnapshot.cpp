#include "qspch.h"
#include "EntitySnapshot.h"

#include "Quelos/Scenes/Scene.h"

namespace Quelos {
    static void SnapshotActor(
        Serialization::BinaryWriter& writer,
        ComponentRegistry& registry,
        flecs::world& world,
        const Actor& actor
    ) {
        writer.Write(actor.GetActorID());

        const std::string_view name = actor.GetName();

        const auto len = static_cast<uint32_t>(name.size());
        writer.Write(len);
        writer.WriteBytes(
            std::as_bytes(std::span(name.data(), name.size()))
        );

        Vec<byte> entityComponentsBuffer;
        Serialization::BinaryWriter entityComponentWriter(entityComponentsBuffer);
        uint32_t componentCount = 0;

        ActorID parentId;
        if (const Actor parent = actor.GetParent(); parent.IsValid()) {
            parentId = parent.GetActorID();
        }

        writer.Write(parentId);
        writer.Write(actor.Get<ChildOrder>().Value);

        Vec<byte> tempComponentData;
        actor.GetInternalID().each([&](const flecs::id id) {
            const SerializableComponentInfo* componentInfo = registry.GetSerializableComponentInfo(id);
            if (!componentInfo) {
                return;
            }

            void* ptr = actor.GetMut(id);
            if (!ptr) {
                return;
            }

            tempComponentData.clear();
            Serialization::BinaryWriter tempWriter(tempComponentData);

            Serialization::BinaryWriteArchive archive(tempWriter);

            componentInfo->SerializeBinaryWriteFunc(archive, ptr);
            const uint64_t size = tempComponentData.size();

            entityComponentWriter.Write(componentInfo->Guid);
            entityComponentWriter.Write(size);
            entityComponentWriter.WriteBytes(tempComponentData);
            componentCount++;
        });

        writer.Write(componentCount);
        writer.WriteBytes(entityComponentsBuffer);

        writer.Write(static_cast<uint32_t>(world.count(flecs::ChildOf, actor.GetInternalID())));
        actor.GetInternalID().children([&](const flecs::entity child) {
            SnapshotActor(writer, registry, world, child);
        });
    }

    ActorSnapshot ActorSnapshot::Create(const Ref<Scene>& scene, const ActorID actorId) {
        ActorSnapshot snapshot;

        const Actor entity = scene->GetActor(actorId);
        Serialization::BinaryWriter writer(snapshot.Data);

        SnapshotActor(writer, scene->GetComponentRegistry(), scene->GetWorld(), entity);

        return snapshot;
    }

    static Actor LoadActor(
        const Ref<Scene>& scene,
        Serialization::BinaryReader& reader,
        ComponentRegistry& registry,
        flecs::world& world
    ) {
        const auto actorIdResult = reader.Read<ActorID>();
        if (!actorIdResult) {
            QS_ERROR_TAG(
                "EntitySnapshot::Load",
                "Couldn't read the ActorID! skipping"
            );

            return {};
        }

        ActorID actorId = actorIdResult.value();

        std::string_view name;
        if (const std::optional<uint32_t> nameLength = reader.Read<uint32_t>()) {
            if (const auto nameBytes = reader.ReadBytes(*nameLength)) {
                name = std::string_view(reinterpret_cast<const char*>(nameBytes.value().data()), nameBytes->size());
            }
        }

        if (name.empty()) {
            QS_ERROR_TAG(
                "EntitySnapshot::Load",
                "Couldn't read the name of the actor... Can't create an actor with no name! '{}!",
                static_cast<uint64_t>(actorId)
            );

            return {};
        }

        const Actor entity = scene->CreateActor(actorId, name);

        if (const auto parentIdResult = reader.Read<ActorID>(); !parentIdResult) {
            QS_ERROR_TAG(
                "EntitySnapshot::Load",
                "Couldn't read parent ActorID in actor '{}({})!",
                name,
                static_cast<uint64_t>(actorId)
            );
        }
        else {
            if (ActorID parentId = parentIdResult.value()) {
                Actor parent = scene->GetActor(parentId);
                entity.SetParent(parent);
            }
        }

        if (const auto childOrder = reader.Read<uint64_t>()) {
            entity.Set(ChildOrder{childOrder.value()});
        }

        if (const auto componentCount = reader.Read<uint32_t>(); !componentCount) {
            QS_ERROR_TAG(
                "EntitySnapshot::Load",
                "Unknown component count in actor '{}({})!",
                name,
                static_cast<uint64_t>(actorId)
            );
        }
        else {
            for (uint32_t i = 0; i < componentCount.value(); i++) {
                const auto guidResult = reader.Read<ActorID>();
                if (!guidResult) {
                    QS_ERROR_TAG("EntitySnapshot::Load", "Unknown component type in scene!");
                    continue;
                }

                ComponentID componentId(guidResult.value());

                const SerializableComponentInfo* typeInfo = registry.GetSerializableComponentInfo(componentId);

                if (!typeInfo) {
                    QS_ERROR_TAG(
                        "EntitySnapshot::Load",
                        "Unknown component type in actor '{}({})'!",
                        name,
                        static_cast<uint64_t>(actorId)
                    );

                    continue;
                }

                // Serialized blob size
                const auto blobSize = reader.Read<uint64_t>();
                if (!blobSize) {
                    QS_ERROR_TAG(
                        "EntitySnapshot::Load",
                        "Couldn't read component '{}({})' blob size",
                        typeInfo->Name, static_cast<uint64_t>(typeInfo->Guid)
                    );

                    continue;
                }

                const auto blobResult = reader.ReadBytes(blobSize.value());
                if (!blobResult) {
                    QS_ERROR_TAG(
                        "EntitySnapshot::Load",
                        "Couldn't read component '{}({})' blob",
                        typeInfo->Name, static_cast<uint64_t>(typeInfo->Guid)
                    );
                }

                std::span<const std::byte> blob = blobResult.value();

                // Allocate component
                void* componentPtr = ecs_ensure_id(
                    world.c_ptr(),
                    entity.GetInternalID(),
                    typeInfo->RuntimeID,
                    typeInfo->Size
                );

                if (!componentPtr) {
                    QS_ERROR_TAG(
                        "EntitySnapshot::Load",
                        "Failed to ensure component '{}({})'!",
                        typeInfo->Name, static_cast<uint64_t>(typeInfo->Guid)
                    );

                    continue;
                }

                // Deserialize
                Serialization::BinaryReader subReader(blob);
                Serialization::BinaryReadArchive archive(subReader);
                typeInfo->SerializeBinaryReadFunc(archive, componentPtr);
            }
        }

        const auto childCountResult = reader.Read<uint32_t>();
        if (!childCountResult) {
            QS_ERROR_TAG(
                "EntitySnapshot::Load",
                "Failed to read child count '{}({})'!",
                name, static_cast<uint64_t>(actorId)
            );

            return entity;
        }

        for (uint32_t i = 0; i < childCountResult.value(); i++) {
            LoadActor(scene, reader, registry, world);
        }

        return entity;
    }

    Actor ActorSnapshot::Load(const Ref<Scene>& scene, const ActorSnapshot& snapshot) {
        Serialization::BinaryReader reader(snapshot.Data);
        return LoadActor(scene, reader, scene->GetComponentRegistry(), scene->GetWorld());
    }
}
