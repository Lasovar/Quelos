#include "qspch.h"
#include "EntitySnapshot.h"

#include "Quelos/Scenes/Scene.h"

namespace Quelos {
    static void SnapshotEntity(
        Serialization::BinaryWriter& writer,
        ComponentRegistry& registry,
        flecs::world& world,
        const Entity& actor
    ) {
        writer.Write(actor.Get<EntityID>());

        const std::string_view name = actor.GetName();

        const auto len = static_cast<uint32_t>(name.size());
        writer.Write(len);
        writer.WriteBytes(
            std::as_bytes(std::span(name.data(), name.size()))
        );

        Vec<byte> entityComponentsBuffer;
        Serialization::BinaryWriter entityComponentWriter(entityComponentsBuffer);
        uint32_t componentCount = 0;

        EntityID parentId;
        if (const Entity parent = actor.GetParent(); parent.IsValid()) {
            parentId = parent.Get<EntityID>();
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
            SnapshotEntity(writer, registry, world, child);
        });
    }

    EntitySnapshot EntitySnapshot::Create(const Ref<Scene>& scene, const EntityID entityId) {
        EntitySnapshot snapshot;

        const Actor entity = scene->GetActor(entityId);
        Serialization::BinaryWriter writer(snapshot.Data);

        SnapshotEntity(writer, scene->GetComponentRegistry(), scene->GetWorld(), entity);

        return snapshot;
    }

    static Entity LoadEntity(
        const Ref<Scene>& scene,
        Serialization::BinaryReader& reader,
        ComponentRegistry& registry,
        flecs::world& world
    ) {
        const auto actorIdResult = reader.Read<EntityID>();
        if (!actorIdResult) {
            QS_ERROR_TAG(
                "EntitySnapshot::Load",
                "Couldn't read the ActorID! skipping"
            );

            return {};
        }

        EntityID actorId = actorIdResult.value();

        std::string_view name;
        if (const std::optional<uint32_t> nameLength = reader.Read<uint32_t>()) {
            if (auto nameBytes = reader.ReadBytes(*nameLength); !nameBytes.empty()) {
                name = std::string_view(reinterpret_cast<const char*>(nameBytes.data()), nameBytes.size());
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

        if (const auto parentIdResult = reader.Read<EntityID>(); !parentIdResult) {
            QS_ERROR_TAG(
                "EntitySnapshot::Load",
                "Couldn't read parent ActorID in actor '{}({})!",
                name,
                static_cast<uint64_t>(actorId)
            );
        }
        else {
            if (EntityID parentId = parentIdResult.value()) {
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
                const auto guidResult = reader.Read<EntityID>();
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
                        typeInfo->FullName, static_cast<uint64_t>(typeInfo->Guid)
                    );

                    continue;
                }

                const auto componentBlob = reader.ReadBytes(blobSize.value());
                if (componentBlob.empty()) {
                    QS_ERROR_TAG(
                        "EntitySnapshot::Load",
                        "Couldn't read component '{}({})' blob",
                        typeInfo->FullName, static_cast<uint64_t>(typeInfo->Guid)
                    );
                }

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
                        typeInfo->FullName, static_cast<uint64_t>(typeInfo->Guid)
                    );

                    continue;
                }

                // Deserialize
                Serialization::BinaryReader componentBlobReader(componentBlob);
                static HashMap<uint64_t, BufferView> fieldsMap;

                fieldsMap.clear();
                while (componentBlobReader.HasRemaining()) {
                    auto fieldHash = componentBlobReader.Read<uint64_t>();
                    auto fieldSize = componentBlobReader.Read<uint64_t>();

                    if (!fieldHash || !fieldSize) {
                        QS_CORE_ERROR_TAG(
                            "EntitySnapshot::LoadEntity",
                            "Failed to read field data for component {}! invalid field hash or size",
                            typeInfo->FullName
                        );

                        break;
                    }

                    fieldsMap[fieldHash.value()] = componentBlobReader.ReadBytes(fieldSize.value());
                }

                Serialization::BinaryReadArchive archive(fieldsMap);
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
            LoadEntity(scene, reader, registry, world);
        }

        return entity;
    }

    Entity EntitySnapshot::Load(const Ref<Scene>& scene, const EntitySnapshot& snapshot) {
        Serialization::BinaryReader reader(snapshot.Data);
        return LoadEntity(scene, reader, scene->GetComponentRegistry(), scene->GetWorld());
    }
}
