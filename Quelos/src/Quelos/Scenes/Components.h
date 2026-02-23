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
        static void Serialize(TArchive& archive, CameraComponent& data) {
            SceneCamera::ProjectionType projectionType = data.Camera.GetProjectionType();
            float persFov = data.Camera.GetPerspectiveVerticalFOV();
            float persNear = data.Camera.GetPerspectiveNearClip();
            float persFar = data.Camera.GetPerspectiveFarClip();
            float orthoSize = data.Camera.GetOrthographicSize();
            float orthoNear = data.Camera.GetOrthographicNearClip();
            float orthoFar = data.Camera.GetOrthographicFarClip();

            archive.Field("projection", projectionType);
            archive.Field("lens.fov", persFov);
            archive.Field("lens.near", persNear);
            archive.Field("lens.far", persFar);

            archive.Field("ortho.size", orthoSize);
            archive.Field("ortho.near", orthoNear);
            archive.Field("ortho.far", orthoFar);

            if constexpr (TArchive::IsLoading) {
                data.Camera.SetProjectionType(projectionType);
                data.Camera.SetPerspectiveVerticalFOV(persFov);
                data.Camera.SetPerspectiveNearClip(persNear);
                data.Camera.SetPerspectiveFarClip(persFar);
                data.Camera.SetOrthographicSize(orthoSize);
                data.Camera.SetOrthographicNearClip(orthoNear);
                data.Camera.SetOrthographicFarClip(orthoFar);

                data.Camera.RecalculateProjection();
            }
        }
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
            }
            else {
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
    struct ComponentGroup { };

    using AllComponents = ComponentGroup<TransformComponent, CameraComponent, MeshComponent>;
}
