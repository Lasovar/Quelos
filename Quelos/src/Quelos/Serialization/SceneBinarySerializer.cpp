#include "qspch.h"
#include "SceneBinarySerializer.h"

#include "Quelos/Scenes/ComponentRegistery.h"
#include "Quelos/Scenes/Components.h"
#include "Quelos/Scenes/Scene.h"
#include "Quelos/Serialization/BinaryWriter.h"

namespace Quelos::Serialization {
    void SceneBinarySerializer::Deserialize(const Ref<Scene>& scene, const std::filesystem::path& path) {
        flecs::world& world = scene->GetWorld();
        ComponentRegistry& registry = scene->GetComponentRegistry();

        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file) {
            throw std::runtime_error("Failed to open scene file");
        }

        const size_t fileSize = file.tellg();
        Vec<byte> buffer(fileSize);

        file.seekg(0);
        file.read(reinterpret_cast<char*>(buffer.data()), fileSize);

        BinaryReader reader(buffer);

        // Read header
        SceneHeader header = reader.Read<SceneHeader>().value();

        if (header.Magic != SceneHeader().Magic) {
            throw std::runtime_error("Invalid scene magic");
        }

        for (uint64_t i = 0; i < header.EntityCount; ++i) {
            EntityID entityId(reader.Read<uint64_t>().value());
            uint32_t nameLength = reader.Read<uint32_t>().value();
            auto nameBytes = reader.ReadBytes(nameLength).value();

            std::string name(
                reinterpret_cast<const char*>(nameBytes.data()),
                nameLength
            );

            Entity entity = scene->CreateEntity(entityId, name.c_str());

            uint32_t componentCount = reader.Read<uint32_t>().value();
            for (uint32_t c = 0; c < componentCount; ++c) {
                ComponentID guid(reader.Read<uint64_t>().value());

                auto typeInfo = registry.GetSerializableComponentInfo(guid);

                if (!typeInfo.Guid.IsValid()) {
                    throw std::runtime_error("Unknown component type in scene");
                }

                // Serialized blob size
                uint64_t blobSize = reader.Read<uint64_t>().value();

                std::span<const std::byte> blob = reader.ReadBytes(blobSize).value();

                // Allocate component
                void* componentPtr = ecs_ensure_id(world.c_ptr(), entity.GetID(), typeInfo.RuntimeID, 0);

                if (!componentPtr) {
                    throw std::runtime_error("Failed to ensure component");
                }

                // Deserialize
                BinaryReader subReader(blob);
                BinaryReadArchive archive(subReader);
                typeInfo.SerializeBinaryReadFunc(archive, componentPtr);
            }
        }
    }

    void SceneBinarySerializer::Serialize(const Ref<Scene>& scene, const std::filesystem::path& path) {
        flecs::world& world = scene->GetWorld();
        ComponentRegistry& registry = scene->GetComponentRegistry();
        const auto& types = registry.GetSerializableComponents();

        std::vector<std::byte> finalBuffer;
        BinaryWriter finalWriter(finalBuffer);

        // Header
        SceneHeader header{};
        header.ComponentTypeCount = types.size();
        header.EntityCount = world.count<RuntimeTag>();
        QS_CORE_INFO("Entity count: {}", header.EntityCount);

        finalWriter.Write(header);

        auto guidLookup = world.query_builder<RuntimeTag>().build();
        Vec<Pair<EntityID, flecs::entity>> entities;
        entities.reserve(guidLookup.count());

        guidLookup.each([&entities](const flecs::entity entity, const RuntimeTag& tag) {
            entities.emplace_back(tag.ID, entity);
        });

        std::ranges::sort(
            entities,
            [](const Pair<EntityID, flecs::entity>& a, const Pair<EntityID, flecs::entity>& b) {
                return a.first < b.first;
            }
        );

        for (auto& [guid, entity] : entities) {
            const ecs_entity_t entityId = entity.id();

            finalWriter.Write(guid);
            const flecs::string_view name = entity.name();
            const auto len = static_cast<uint32_t>(name.size());
            finalWriter.Write(len);
            finalWriter.WriteBytes(
                std::as_bytes(std::span(name.c_str(), name.size()))
            );

            std::vector<std::byte> entityComponentsBuffer;
            BinaryWriter entityComponentWriter(entityComponentsBuffer);
            uint32_t componentCount = 0;

            entity.each([&](const flecs::id id) {
                SerializableComponentInfo* componentInfo = registry.GetSerializableComponentInfo(id);
                if (!componentInfo) {
                    return;
                }

                void* ptr = ecs_get_mut_id(world.c_ptr(), entityId, id);
                if (!ptr) {
                    return;
                }

                std::vector<std::byte> temp;
                BinaryWriter tempWriter(temp);

                BinaryWriteArchive archive(tempWriter);

                componentInfo->SerializeBinaryWriteFunc(archive, ptr);
                const uint64_t size = temp.size();

                entityComponentWriter.Write(componentInfo->Guid);
                entityComponentWriter.Write(size);
                entityComponentWriter.WriteBytes(temp);
                componentCount++;
            });

            finalWriter.Write(componentCount);
            finalWriter.WriteBytes(entityComponentsBuffer);
        }

        // Disk write
        std::ofstream file(path, std::ios::binary);
        file.write(
            reinterpret_cast<const char*>(finalBuffer.data()),
            static_cast<std::streamsize>(finalBuffer.size())
        );
    }
}
