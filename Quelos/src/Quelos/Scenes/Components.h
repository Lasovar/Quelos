#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "Quelos/Serialization/BinaryWriter.h"
#include "Quelos/Serialization/Serializer.h"

#include "Quelos/AssetManager/Assets/Mesh.h"

#include "Quelos/Renderer/Material.h"
#include "Quelos/Renderer/SceneCamera.h"
#include "Quelos/Renderer/Shader.h"

namespace Quelos {
    using EntityID = GUID64;

    class IndexBuffer;
    class VertexBuffer;

    struct RuntimeTag {
        EntityID ID;
    };

    struct TransformComponent {
        glm::vec3 Position;
        glm::quat Rotation;
        glm::vec3 Scale;

        template <typename TArchive>
        static void Serialize(TArchive& archive, TransformComponent& data) {
            archive.Field("position", data.Position);
            archive.Field("rotation", data.Rotation);
            archive.Field("scale", data.Scale);
        }
    };

    struct CameraComponent {
        SceneCamera Camera;

        template <typename TArchive>
        static void Serialize(TArchive& writer, CameraComponent& data) { }
    };

    struct MeshComponent {
        Ref<Mesh> MeshData;
        Ref<Material> MaterialData;

        template <typename TArchive>
        static void Serialize(TArchive& archive, MeshComponent& data) {
            if constexpr (TArchive::IsSaving) {
                auto& vertices = data.MeshData->GetVertices();
                auto& indices = data.MeshData->GetIndices();

                archive.FieldVector("vertices", vertices);
                archive.FieldVector("indices", indices);
            } else {
                std::vector<PosColorVertex> vertices;
                archive.FieldVector("vertices", vertices);

                std::vector<uint16_t> indices;
                archive.FieldVector("indices", indices);

                data.MeshData = CreateRef<Mesh>(std::move(vertices), std::move(indices));
                data.MaterialData = CreateRef<Material>(Shader::Create("vs_cubes.bin", "fs_cubes.bin"));
            }
        }
    };

    template <typename... Component>
    struct ComponentGroup {
    };

    using AllComponents = ComponentGroup<TransformComponent, CameraComponent, MeshComponent>;
}
